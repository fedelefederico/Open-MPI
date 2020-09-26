#include "mpi.h"
#include <stdio.h>

#define DIM 20

int main(int argc, char* argv[]){
    int worldrank, nproc;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    if(worldrank==0){
        int A[DIM];
        FILE*f=fopen("vett.txt","r");
        for(int i=0; i<DIM; i++){
            fscanf(f,"%d",&A[i]);
        }
        int k=2;
        int procchunks=(DIM/nproc)/k;
        MPI_Datatype chunksType;
        MPI_Type_vector(procchunks,k,nproc*k,MPI_INT,&chunksType);
        MPI_Type_commit(&chunksType);

        for(int i=0; i<nproc; i++){
            MPI_Send(&A[i*k],1,chunksType,i,0,MPI_COMM_WORLD);
        }
    }

    int V[DIM/nproc];
    MPI_Status status;
    MPI_Recv(V,DIM/nproc,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    printf("P%d, V:\n",worldrank);
    for(int i=0; i<DIM/nproc; i++){
        printf("%d ",V[i]);
    }

    /*
    Ordinamento crescente
    */
   for(int i=0; i<DIM/nproc; i++){
       int min=V[i];
       int pos=i;
       for(int j=i+1; j<DIM/nproc; j++){
           if(V[j]<V[i]){
               min=V[j];
               pos=j;
           }
       }
       int tmp=V[i];
       V[i]=V[pos];
       V[pos]=tmp;
   }
   printf("\nP%d, V crescente\n",worldrank);
   for(int i=0; i<DIM/nproc; i++){
       printf("%d<",V[i]);
   }
   printf(" \n");

    /*
    Creazione gruppi
    */
   int colore;
   if(worldrank%2==0){ //processi rango pari
       colore=0;
   } else colore=1;

   MPI_Comm newcomm;
   MPI_Comm_split(MPI_COMM_WORLD,colore,worldrank,&newcomm);
   int newrank;
   MPI_Comm_rank(newcomm,&newrank);

   if(colore==0){
       int Vsum[DIM/nproc];
       MPI_Reduce(V,Vsum,DIM/nproc,MPI_INT,MPI_SUM,0,newcomm);
       if(newrank==0){
           MPI_Send(Vsum,DIM/nproc,MPI_INT,nproc-1,0,MPI_COMM_WORLD);
       }
   } else{
       int Vprod[DIM/nproc];
       MPI_Reduce(V,Vprod,DIM/nproc,MPI_INT,MPI_PROD,0,newcomm);
       if(newrank==0){
           MPI_Send(Vprod,DIM/nproc,MPI_INT,nproc-1,1,MPI_COMM_WORLD);
       }
   }

   if(worldrank==nproc-1){
       int Vsum[DIM/nproc],Vprod[DIM/nproc];
       MPI_Recv(Vsum,DIM/nproc,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
       MPI_Recv(Vprod,DIM/nproc,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&status);
       printf("P%d, ricevuti:\n",worldrank);
       for(int i=0; i<DIM/nproc; i++){
           printf("%d+",Vsum[i]);
       }
       printf(" \n");
       for(int i=0; i<DIM/nproc; i++){
           printf("%d*",Vprod[i]);
       }
       printf(" \n");
   }

   /*
    Chi ha il massimo dei massimi lo manda a P0
   */
  int localmax=V[0];
  for(int i=1; i<DIM/nproc; i++){
      if(V[i]>localmax) localmax=V[i];
  }
  int globalmax;
  MPI_Allreduce(&localmax,&globalmax,1,MPI_INT,MPI_MAX,MPI_COMM_WORLD);
  if(localmax==globalmax){
      MPI_Send(V,DIM/nproc,MPI_INT,0,0,MPI_COMM_WORLD);
  }

  if(worldrank==0){
      int Vmax[DIM/nproc];
      MPI_Recv(Vmax,DIM/nproc,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
      printf("\nP%d, Vmax:\n",worldrank);
      for(int i=0; i<DIM/nproc; i++){
          printf("%d*",Vmax[i]);
      }
      printf(" \n");
  }

   MPI_Finalize();
}