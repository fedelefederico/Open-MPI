#include "mpi.h"
#include <stdio.h>
#include <math.h>

#define DIM 6

int main(int argc, char* argv[]){
    int worldrank,nproc;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    MPI_Comm cart;
    int dims[2]={sqrt(nproc),sqrt(nproc)};
    int periods[2]={1,1};
    MPI_Cart_create(MPI_COMM_WORLD,2,dims,periods,0,&cart);

    int q=sqrt(nproc);
    int blocksize=DIM/q;

    if(worldrank==0){
        int A[DIM][DIM];
        FILE*f=fopen("m.txt","r");
        for(int i=0; i<DIM; i++){
            for(int j=0; j<DIM; j++){
                fscanf(f,"%d",&A[i][j]);
            }
        }

        MPI_Datatype blockType;
        MPI_Type_vector(blocksize,blocksize,DIM,MPI_INT,&blockType);
        MPI_Type_commit(&blockType);

        for(int i=0; i<q; i++){
            for(int j=0; j<q; j++){
                int dest;
                int coords[2]={i,j};
                MPI_Cart_rank(cart,coords,&dest);
                MPI_Send(&A[i*blocksize][j*blocksize],1,blockType,dest,0,cart);
            }
        }
    }

    MPI_Status status;
    int Alocal[blocksize][blocksize];
    MPI_Recv(Alocal,blocksize*blocksize,MPI_INT,0,0,cart,&status);
    printf("P%d, Alocal:\n",worldrank);
    for(int i=0; i<blocksize; i++){
        for(int j=0; j<blocksize; j++){
            printf("%d ",Alocal[i][j]);
        }
        printf("\n");
    }

    /*
    Ribaltamento righe di A
    */
   for(int i=0; i<blocksize/2; i++){
       int tmp[blocksize];
       for(int j=0; j<blocksize; j++){
           tmp[j]=Alocal[i][j];
       }
       for(int j=0; j<blocksize; j++){
           Alocal[i][j]=Alocal[blocksize-1-i][j];
       }
       for(int j=0; j<blocksize; j++){
           Alocal[blocksize-1-i][j]=tmp[j];
       }
   }

   printf("P%d, Alocal ribaltata:\n",worldrank);
   for(int i=0; i<blocksize; i++){
       for(int j=0; j<blocksize; j++){
           printf("%d ",Alocal[i][j]);
       }
       printf("\n");
   }

   /*
    Shift
   */
  int source;
  int dest;
  MPI_Cart_shift(cart,0,2,&source,&dest);
  MPI_Sendrecv_replace(Alocal,blocksize*blocksize,MPI_INT,dest,0,source,0,cart,&status);

   printf("P%d, Alocal dopo shift:\n",worldrank);
   for(int i=0; i<blocksize; i++){
       for(int j=0; j<blocksize; j++){
           printf("%d ",Alocal[i][j]);
       }
       printf("\n");
   }

   /*
    Minimo diag secondaria di M
   */

  int color=0;
  int coords[2];
  int cartrank;
  MPI_Comm_rank(cart,&cartrank);
  MPI_Cart_coords(cart,cartrank,2,coords);
  if(coords[0]+coords[1]==q-1){
      color=1;
  }

  MPI_Comm diag_comm;
  MPI_Comm_split(cart,color,cartrank,&diag_comm);
  int newrank;
  MPI_Comm_rank(diag_comm,&newrank);

  if(color==1){
      int localmin;
      for(int i=0; i<blocksize; i++){
        if(Alocal[i][blocksize-1-i]<localmin) localmin=Alocal[i][blocksize-1-i];          
      }

      struct{
          int value;
          int rank;
      } in, out;

      in.value=localmin;
      in.rank=newrank;

      MPI_Reduce(&in,&out,1,MPI_2INT,MPI_MAXLOC,0,diag_comm);
      if(newrank==0){
          printf("P%d, minimo globale: %d\n",worldrank,out.value);
      }
  }

  MPI_Finalize();

}