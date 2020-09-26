#include "mpi.h"
#include <stdio.h>
#define DIM 12

int main(int argc, char* argv[]){
    int worldrank, nproc;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int k=DIM/nproc;

    if(worldrank==0){
        FILE*f=fopen("vettori.txt","r");
        int A[DIM],B[DIM];
        for(int i=0; i<DIM; i++){
            fscanf(f,"%d",&A[i]);
        }
        for(int i=0; i<DIM; i++){
            fscanf(f,"%d",&B[i]);
        }

        MPI_Datatype chunkType;
        MPI_Type_vector(DIM/nproc,1,nproc,MPI_INT,&chunkType);
        MPI_Type_commit(&chunkType);

        for(int i=0; i<nproc; i++){
            MPI_Send(&A[i*k],k,MPI_INT,i,0,MPI_COMM_WORLD);
            MPI_Send(&B[i],1,chunkType,i,1,MPI_COMM_WORLD);
        }
    }

    int Ai[k],Bi[k];
    MPI_Status status;
    MPI_Recv(Ai,k,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    MPI_Recv(Bi,k,MPI_INT,0,1,MPI_COMM_WORLD,&status);

    printf("\nP%d, Ai:\n",worldrank);
    for(int i=0; i<k; i++){
            printf("%d ",Ai[i]);
    }
    printf("\nP%d, Bi:\n",worldrank);
    for(int i=0; i<k; i++){
        printf("%d ",Bi[i]);
    }

    /*
    Amax
    */
   int Amax[k];
   MPI_Allreduce(Ai,Amax,k,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
   printf("\nP%d, Amax:\n",worldrank);
   for(int i=0; i<k; i++){
       printf("%d ",Amax[i]);
   }
   /*
    Ci
   */
   int Ci[k];

   //prodotto scalare
   int prod=0;
   for(int i=0; i<k; i++){
       prod=prod+Ai[i]*Bi[i];
   }

   for(int i=0; i<k; i++){
       Ci[i]=prod*Amax[i];
   }
   printf("\nP%d, prod=%d, Ci:\n",worldrank,prod);
   for(int i=0; i<k; i++){
       printf("%d ",Ci[i]);
   }

   /*
    Allgather 
   */
   int M[k][k];
   MPI_Allgather(Ci,k,MPI_INT,M,k,MPI_INT,MPI_COMM_WORLD);
   printf("\nP%d, M:\n",worldrank);
   for(int i=0; i<k; i++){
       for(int j=0; j<k; j++){
           printf("%d ",M[i][j]);
       }
       printf("\n");
   }

   MPI_Finalize();
}