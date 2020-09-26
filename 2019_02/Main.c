#include "mpi.h"
#include <stdio.h>
#include <math.h>

#define DIM 12

int main(int argc, char* argv[]){
    int worldrank, nproc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int k=DIM/nproc;

    if(worldrank==0){
        int A[DIM][DIM];
        FILE*f=fopen("A.txt","r");
        for(int i=0; i<DIM; i++){
            for(int j=0; j<DIM; j++){
                fscanf(f,"%d",&A[i][j]);
            }
        }

        /*
        printf("P%d, A:\n");
        for(int i=0; i<DIM; i++){
            for(int j=0; j<DIM; j++){
                printf("%d ",A[i][j]);
            }
            printf("\n");
        }*/

        MPI_Datatype rowsType;
        MPI_Type_vector(k,DIM,nproc*DIM,MPI_INT,&rowsType);
        MPI_Type_commit(&rowsType);

        for(int i=0; i<nproc; i++){
            MPI_Send(&A[i][0],1,rowsType,i,0,MPI_COMM_WORLD);
        }
    }

    MPI_Status status;
    int Alocal[k][DIM];
    MPI_Recv(Alocal,k*DIM,MPI_INT,0,0,MPI_COMM_WORLD,&status);

    printf("P%d, Alocal:\n",worldrank);
    for(int i=0; i<k; i++){
        for(int j=0; j<DIM; j++){
            printf("%d,",Alocal[i][j]);
        }
        printf("\n");
    }

    /*
    topologia
    */

   MPI_Comm cart;
   int dims[2]={sqrt(nproc),sqrt(nproc)};
   int periods[2]={1,1};
   MPI_Cart_create(MPI_COMM_WORLD,2,dims,periods,0,&cart);
   int cartrank;
   MPI_Comm_rank(cart,&cartrank);
   int coords[2];
   MPI_Cart_coords(cart,cartrank,2,coords);

   MPI_Comm row_comm;
   MPI_Comm_split(cart,coords[0],cartrank,&row_comm);
   
   int rcv[k][DIM];
   if(coords[0]==coords[1]){
     for(int i=0; i<k; i++){
       for(int j=0; j<DIM; j++){
           rcv[i][j]=Alocal[i][j];
       }
     }  
   }

   MPI_Bcast(rcv,k*DIM,MPI_INT,coords[0],row_comm);

   printf("P%d, rcv:\n",worldrank);
   for(int i=0; i<k; i++){
       for(int j=0; j<DIM; j++){
           printf("%d ",rcv[i][j]);
       }
       printf("\n");
   }

   /*
    Calcolo C(k,k)
   */

  int trasposta[DIM][k];
  for(int i=0; i<k; i++){
      for(int j=0; j<DIM; j++){
          trasposta[j][i]=rcv[i][j];
      }
  }

  printf("P%d, trasposta:\n",worldrank);
  for(int i=0; i<DIM; i++){
      for(int j=0; j<k; j++){
          printf("%d,",trasposta[i][j]);
      }
      printf("\n");
  }

  int C[k][k];
  for(int i=0; i<k; i++){
      for(int j=0; j<k; j++){
          C[i][j]=0;
      }
  }
  for(int i=0; i<k; i++){
      for(int j=0; j<k; j++){
          for(int h=0; h<DIM; h++){
              C[i][j]=C[i][j]+Alocal[i][h]*trasposta[h][j];
          }
      }
  }

  printf("P%d, C:\n",worldrank);
  for(int i=0; i<k; i++){
      for(int j=0; j<k; j++){
          printf("%d ",C[i][j]);
      }
      printf("\n");
  }

  /*
    Massimi diagonale
  */

  int vincitore=0;
  MPI_Comm newcomm=MPI_COMM_WORLD;
  int newrank=worldrank;

  struct{
      int value;
      int rank;
  } in, out;

  int massimi[k];

  for(int t=0; t<k; t++){
      MPI_Comm_split(newcomm,vincitore,newrank,&newcomm);
      MPI_Comm_rank(newcomm,&newrank);
      if(vincitore==0){
          in.value=C[t][t];
          in.rank=newrank;
          MPI_Allreduce(&in,&out,1,MPI_2INT,MPI_MAXLOC,newcomm);
          if(newrank==out.rank){
              vincitore=1;
              printf("P%d, ho vinto la gara %d e non partecipo alle successive",worldrank,t);
              //comunico a tutti chi ha vinto
              for(int i=0; i<nproc; i++){
                  MPI_Send(&worldrank,1,MPI_INT,i,0,MPI_COMM_WORLD);
              }
              massimi[t]=out.value;
          }
      }

      int root;
      MPI_Recv(&root,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
      MPI_Bcast(&massimi[t],1,MPI_INT,root,MPI_COMM_WORLD);
  }

  printf("P%d, massimi:\n",worldrank);
  for(int i=0; i<k; i++){
      printf("%d ",massimi[i]);
  }
  printf("\n");

  MPI_Finalize();

}