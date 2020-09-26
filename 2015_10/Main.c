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
        int V[DIM];
        FILE*f=fopen("vett.txt","r");
        for(int i=0; i<DIM; i++){
            fscanf(f,"%d",&V[i]);
        }

        MPI_Datatype chunksTesta;
        MPI_Type_vector( (DIM/nproc),1,nproc,MPI_INT,&chunksTesta);
        MPI_Type_commit(&chunksTesta);

        MPI_Datatype chunksCoda;
        MPI_Type_vector( (DIM/nproc),1,-nproc,MPI_INT, &chunksCoda);
        MPI_Type_commit(&chunksCoda);

        for(int i=0; i<nproc; i++){
            MPI_Send(&V[i],1,chunksTesta,i,0,MPI_COMM_WORLD);
            MPI_Send(&V[DIM-1-i],1,chunksCoda,i,1,MPI_COMM_WORLD);
        }
    }

    int A[k], B[k];
    MPI_Status status;
    MPI_Recv(A,DIM/nproc,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    MPI_Recv(B,DIM/nproc,MPI_INT,0,1,MPI_COMM_WORLD,&status);

    printf("P%d, A:\n",worldrank);
    for(int i=0; i<DIM/nproc; i++){
        printf("%d>",A[i]);
    }
    printf("\nP%d, B:\n",worldrank);
    for(int i=0; i<DIM/nproc; i++){
        printf("%d<",B[i]);
    }

    /*
    Shift dei vettori
    */

   MPI_Comm newcomm;
   int dims[1]={nproc};
   int periods[1]={1};
   MPI_Cart_create(MPI_COMM_WORLD,1,dims,periods,0,&newcomm);
   int sourceA,destA,sourceB,destB;

   MPI_Cart_shift(newcomm,0,2,&sourceA,&destA);
   MPI_Cart_shift(newcomm,0,-3,&sourceB,&destB);

   MPI_Sendrecv_replace(A,k,MPI_INT,destA,0,sourceA,0,newcomm,&status);
   MPI_Sendrecv_replace(B,k,MPI_INT,destB,0,sourceB,0,newcomm,&status);

    printf("P%d, nuovo A:\n",worldrank);
    for(int i=0; i<DIM/nproc; i++){
        printf("%d>",A[i]);
    }
    printf("P%d, nuovo B:\n",worldrank);
    for(int i=0; i<DIM/nproc; i++){
        printf("%d<",B[i]);
    }

    /*
    Prodotto scalare
    */

   int prodotto=0;
   for(int i=0; i<k; i++){
       prodotto=prodotto+(A[i]*B[i]);
   }

   printf("\nP%d, prodotto: %d\n",worldrank,prodotto);

   /*
    Il processo con prod MAX raccoglie anche gli altri prod
   */
  struct {
      int value;
      int rank;
  } in,out;

  in.value=prodotto;
  in.rank=worldrank;

  MPI_Allreduce(&in,&out,1,MPI_2INT,MPI_MAXLOC,MPI_COMM_WORLD);

  int prodotti[nproc];
  MPI_Gather(&prodotto,1,MPI_INT,prodotti,1,MPI_INT,out.rank,MPI_COMM_WORLD);

  if(worldrank==out.rank){
      printf("\nP%d, vettore prodotti:\n");
      for(int i=0; i<nproc; i++){
          printf("%d_",prodotti[i]);
      }
  }


  MPI_Finalize();
}