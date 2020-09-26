#include "mpi.h"
#include <stdio.h>

#define DIM 15

int main(int argc, char* argv[]){
    int worldrank,nproc;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);
    
    int k=DIM/nproc;
    int A[DIM][DIM], B[DIM][DIM];
    int Alocal[k][DIM], Blocal[DIM][k];
    if(worldrank==0){
        
        FILE*f=fopen("A.txt","r");
        for(int i=0; i<DIM; i++){
            for(int j=0; j<DIM; j++){
                fscanf(f,"%d",&A[i][j]);
            }
        }
        f=fopen("B.txt","r");
        for(int i=0; i<DIM; i++){
            for(int j=0; j<DIM; j++){
                fscanf(f,"%d",&B[i][j]);
            }
        }
        fclose(f);
    }

    //invio righe
    MPI_Scatter(A,DIM*k,MPI_INT,Alocal,k*DIM,MPI_INT,0,MPI_COMM_WORLD);
    /*printf("P%d, Alocal:\n",worldrank);
    for(int i=0; i<k; i++){
        for(int j=0; j<DIM; j++){
            printf("%d ",Alocal[i][j]);
        }
        printf("\n");
    }*/

    //colonne
    MPI_Datatype colsType;
    MPI_Type_vector(DIM,k,DIM,MPI_INT,&colsType);
    MPI_Type_create_resized(colsType,0,k*sizeof(int),&colsType);
    MPI_Type_commit(&colsType);
    MPI_Scatter(B,1,colsType,Blocal,DIM*k,MPI_INT,0,MPI_COMM_WORLD);
    /*
    printf("P%d, Blocal:\n",worldrank);
    for(int i=0; i<DIM; i++){
        for(int j=0; j<k; j++){
            printf("%d ",Blocal[i][j]);
        }
        printf("\n");
    }
    */

    /*
    C=A*B
    */
    int C[k][k];
    for(int i=0; i<k; i++){
        for(int j=0; j<k; j++){
            C[i][j]=0;
        }
    }
    for(int i=0; i<k; i++){
        for(int j=0; j<k; j++){
            for(int h=0; h<DIM; h++){
                C[i][j]=C[i][j]+Alocal[i][h]*Blocal[h][j];
            }
        }
    }

/*
    printf("P%d, C:\n",worldrank);
    for(int i=0; i<k; i++){
        for(int j=0; j<k; j++){
            printf("%d ",C[i][j]);
        }
        printf("\n");
    }*/

    /*
    Scorrimento colonne: in totale ogni processo invia la colonna i a TUTTI nproc volte
    la prima volta la invia direttamente, le restanti nproc-1 volte la invia in seguito ad una ricezione
    */ 
   MPI_Status status;

   //Colonna i-sima di Pi
   int colonnai[k], colonnarcv[k];
   for(int h=0; h<k; h++){
       colonnai[h]=C[h][worldrank];
   }
   
   int dest=worldrank;
   for(int i=1; i<nproc; i++){ //Al primo giro tutti inviano la colonna i-sima a tutti
        dest=(worldrank+i)%nproc;
        MPI_Send(colonnai,k,MPI_INT,dest,0,MPI_COMM_WORLD);
   }

    /*
    Per nproc-1 volte ricevo nproc-1 colonne
    Ogni volta che ricevo, faccio il confronto e poi invio a tutti
    */
   for(int h=0; h<nproc; h++){ //l'ultimo giro ricevo senza inviare 
       int source=worldrank;
       for(int i=1; i<nproc; i++){
           source=modulo(source-1,nproc);
           MPI_Recv(colonnarcv,k,MPI_INT,source,0,MPI_COMM_WORLD,&status);

           //confronto colonna ed eventuale sostituzione
           int maggiore=1;
           for(int j=0; j<k; j++){
               if(colonnarcv[j]<colonnai[j]) maggiore=0;
           }

           if(maggiore==1){ //sostituzione
               for(int j=0; j<k; j++){
                   colonnai[j]=colonnarcv[j];
               }
           }

           //Reinvio la colonna a tutti
           if(h!=nproc-1){
                int dest=worldrank;
                for(int j=1; j<nproc; j++){
                    dest=(dest+1)%nproc;
                    MPI_Send(colonnai,k,MPI_INT,dest,0,MPI_COMM_WORLD);
                } 
           }
           
       }
   }

    for(int h=0; h<k; h++){
       C[h][worldrank]=colonnai[h];
   }

/*
   printf("P%d dopo sostituzioni:\n",worldrank);
   for(int i=0; i<k; i++){
       for(int j=0; j<k; j++){
           printf("%d ",C[i][j]);
       }
       printf("\n");
   }*/

   /*
    Calcolo il vettore dei massimi della prima riga di C
   */
  int primariga[k];
  for(int i=0; i<k; i++){
      primariga[i]=C[0][i];
  }

  int rigamax[k];
  MPI_Allreduce(primariga,rigamax,k,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
  printf("P%d rigamax:\n",worldrank);
  for(int i=0; i<k; i++){
      printf("%d ",rigamax[i]);
  }
  printf("\nP%d primariga:\n",worldrank);
  for(int i=0; i<k; i++){
      printf("%d ",primariga[i]);
  }
  
  int colore=1; //Quelli che inviano un messaggio a P0
  if(worldrank!=0){
      for(int i=0; i<k; i++){
          if( (rigamax[i]-primariga[i])==0 ) colore=0; 
      }
  }

  //Mi serve a sapere quanti sono quelli con color=1
  MPI_Comm newcomm;
  MPI_Comm_split(MPI_COMM_WORLD,colore,worldrank,&newcomm);
  int sizenewcomm;
  MPI_Comm_size(newcomm,&sizenewcomm);

  if(worldrank!=0 && colore==1){
      printf("\nP%d ho la differenza priva di 0, invio\n",worldrank);
      MPI_Send(primariga, k, MPI_INT, 0, 1,MPI_COMM_WORLD);
  } 

  if(worldrank==0){
      sleep(3);
      int rcvbuff[k];
      for(int i=0; i<sizenewcomm-1; i++){
          MPI_Recv(rcvbuff,k,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&status);
          printf("\nP%d ricevuto:\n",worldrank);
          for(int h=0; h<k; h++){
              printf("%d*",rcvbuff[h]);
          }
      }
  } 
  
  MPI_Finalize();
}


int modulo(int x, int N){
    return (x % N + N) % N;
}