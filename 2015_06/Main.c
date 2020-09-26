#include "mpi.h"
#include <stdio.h>

#define DIM 48

int main(int argc, char* argv[]){
    int worldrank, nproc;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int chunk;
    //Votazione
    int voto=(rand()+worldrank)%2;
    int risultato;
    MPI_Allreduce(&voto,&risultato,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
    if(risultato>nproc/2){
        chunk=DIM/(4*nproc);
    }
    else{
        chunk=DIM/(8*nproc);
    }

    printf("Risultato votazione: chunk=%d\n",chunk);

    if(worldrank==0){
        int V[DIM];
        FILE*f=fopen("v.txt","r");
        for(int i=0; i<DIM; i++){
            fscanf(f,"%d",&V[i]);
        }

        printf("P%d, V:\n",worldrank);
        for(int i=0; i<DIM; i++){
            printf("%d,",V[i]);
        }
        printf("\n");

        MPI_Datatype chunksType;
        MPI_Type_vector((DIM/nproc)/chunk,chunk,nproc*chunk,MPI_INT,&chunksType);
        MPI_Type_commit(&chunksType);

        for(int i=0; i<nproc; i++){
            MPI_Send(&V[i*chunk],1,chunksType,i,0,MPI_COMM_WORLD);
        }
    }

    int Vlocal[DIM/nproc];
    MPI_Status status;
    MPI_Recv(Vlocal,DIM/nproc,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    printf("P%d, Vlocal:\n",worldrank);
    for(int i=0; i<DIM/nproc; i++){
        printf("%d ",Vlocal[i]);
    } 
    printf("\n");

    /*
    Ordinamento 
    */
   for(int i=0; i<DIM/nproc; i++){
       int max=Vlocal[i];
       int pos=i;
       for(int j=i+1; j<DIM/nproc; j++){
           if(Vlocal[j]>max){
               max=Vlocal[j];
               pos=j;
           }
       }

       int tmp=Vlocal[i];
       Vlocal[i]=Vlocal[pos];
       Vlocal[pos]=tmp;    
   }

    printf("\nP%d, Vlocal decrescente:\n",worldrank);
    for(int i=0; i<DIM/nproc; i++){
        printf("%d<",Vlocal[i]);
    }
    printf("\n");

    /*
    Costruzione matrice in A in base all'ordine dato dai massimi dei vettori
    */
   int localmax=Vlocal[0];
   for(int i=1; i<DIM/nproc; i++){
       if(Vlocal[i]>localmax) localmax=Vlocal[i];
   }

   int vincitore=0;
   MPI_Comm newcomm=MPI_COMM_WORLD;
   int newrank=worldrank;

   struct{
       int value;
       int rank;
   } in,out;
   in.value=localmax;
   in.rank=newrank;

   for(int i=0; i<nproc; i++){       
       MPI_Comm_split(newcomm,vincitore,newrank,&newcomm);
       if(!vincitore){ //All'i-sima gara partecipa chi non ha già vinto
            MPI_Comm_rank(newcomm,&newrank);
            in.rank=newrank;
            MPI_Allreduce(&in,&out,1,MPI_2INT,MPI_MAXLOC,newcomm);

            if(newrank==out.rank){ //vincitore
                vincitore=1; //Esco dal gruppo
                MPI_Send(Vlocal,DIM/nproc,MPI_INT,0,i,MPI_COMM_WORLD);
            }
       }
   }

   if(worldrank==0){
       int A[nproc][DIM/nproc];
       for(int i=0; i<nproc; i++){
           MPI_Recv(&A[i][0],DIM/nproc,MPI_INT,MPI_ANY_SOURCE,i,MPI_COMM_WORLD,&status);
       }

       printf("P%d, A:\n",worldrank);
       for(int i=0; i<nproc; i++){
           for(int j=0; j<DIM/nproc; j++){
               printf("%d ",A[i][j]);
           }
           printf("\n");
       }
   }

    MPI_Finalize();
}