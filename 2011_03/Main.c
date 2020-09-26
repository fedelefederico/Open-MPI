#include "mpi.h"
#include <stdio.h>
#include <limits.h>

#define n 4

int main(int argc, char* argv[]){
    int worldrank,nproc;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int A[n][n];
    int B[n/2][n/2];

    int dims[2]={nproc/2,nproc/2};
    int periods[2]={0,0};
    MPI_Comm cart;
    MPI_Cart_create(MPI_COMM_WORLD,2,dims,periods,1,&cart);

     MPI_Datatype blockType;
     
    int size_array[2]={n,n};
    int subsize_array[2]={n/2,n/2};
    int start[2]={0,0};
    //MPI_Type_create_subarray(2,size_array,subsize_array,start,0,MPI_INT,&blockType);
    
   MPI_Type_vector(n/2,n/2,n,MPI_INT,&blockType);
   MPI_Type_create_resized(blockType,0,sizeof(int),&blockType);
   MPI_Type_commit(&blockType);  

    if(worldrank==0){
        FILE*f=fopen("m.txt","r");
        for(int i=0; i<n;i++){
            for(int j=0; j<n; j++){
                fscanf(f,"%d",&A[i][j]);
            }
        }        
/*
        int coords[2];
        int cartrank;

        for(int i=0; i<nproc/2; i++){
            for(int j=0; j<nproc/2; j++){
                coords[0]=i;
                coords[1]=j;
                MPI_Cart_rank(cart,coords,&cartrank);
                MPI_Send(&A[i*(n/2)][j*(n/2)],1,blockType,cartrank,0,cart);
            }
        }   */            
    }

    //Invio alternativo con scatterv
    int index=0;
    int offsets[nproc];
    for(int i=0; i<nproc/2; i++){
        for(int j=0; j<nproc/2; j++){
            offsets[index]=i*(n/2)*n+j*(n/2);
            index++;
        }
    }
    int sendcounts[4]={1,1,1,1};
    MPI_Scatterv(A,sendcounts,offsets,blockType,B,(n/2)*(n/2),MPI_INT,0,MPI_COMM_WORLD); 
    
    MPI_Status status;
    /*
    MPI_Recv(B,(n/2)*(n/2),MPI_INT,MPI_ANY_SOURCE,0,cart,&status);*/

    printf("P%d submatrice:\n",worldrank);
    for(int i=0; i<n/2; i++){
        for(int j=0; j<n/2; j++){
            printf("%d ",B[i][j]);
        }
        printf("\n");
    }

    /*
    Ordino le colonne in maniera tale che la riga 0 sia ordinata
    */
   for(int col1=0; col1<n/2; col1++){
       for(int col2=col1+1; col2<n/2; col2++){
           if(B[0][col1]>B[0][col2]){
               int tmp[n/2];
               for(int k=0;k<n/2;k++){
                   tmp[k]=B[k][col1];
               }
               for(int k=0;k<n/2;k++){
                   B[k][col1]=B[k][col2];
               }
               for(int k=0;k<n/2;k++){
                   B[k][col2]=tmp[k];
               }
           }
       }
   }

   printf("P%d submatrice ordinata:\n",worldrank);
    for(int i=0; i<n/2; i++){
        for(int j=0; j<n/2; j++){
            printf("%d ",B[i][j]);
        }
        printf("\n");
    }

    /*
    Ricostruzione in 0 di A con i sottoblocchi che hanno B(0,0) ordinati in senso orario crescente
    */

   MPI_Datatype subblockType;
   int sizes[2]={n/2,n/2};
   int subsizes[2]={n/2,n/2};
   MPI_Type_create_subarray(2,sizes,subsizes,start,0,MPI_INT,&subblockType);
   MPI_Type_commit(&subblockType);

   int minlocal=B[0][0];
   int minglobal;
   for(int i=0; i<nproc; i++){
       MPI_Allreduce(&minlocal,&minglobal,1,MPI_INT,MPI_MIN,cart);
       if(minlocal==minglobal){
           minlocal=INT_MAX;
           printf("P%d minglobal iterazione %d\n",worldrank,i);
           MPI_Send(B,1,subblockType,0,i,cart);
       }
   }

   if(worldrank==0){  
    
        MPI_Recv(&A[0][0],1,blockType,MPI_ANY_SOURCE,0,cart,&status);
        MPI_Recv(&A[0][n/2],1,blockType,MPI_ANY_SOURCE,1,cart,&status);
        MPI_Recv(&A[n/2][n/2],1,blockType,MPI_ANY_SOURCE,2,cart,&status);
        MPI_Recv(&A[n/2][0],1,blockType,MPI_ANY_SOURCE,3,cart,&status);
       

       printf("Matrice finale\n");
       for(int i=0; i<n; i++){
           for(int j=0; j<n; j++){
               printf("%d ",A[i][j]);
           }
           printf("\n");
       }
   }


    MPI_Finalize();
}