#include "mpi.h"
#include <stdio.h>

#define k 4
#define p 8

int main(int argc,char*argv[]){
    int worldrank,nproc;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int q=p/(nproc-1);
    int pivotrank;

    int estrazione=(rand()+worldrank)%100;
    struct{
        int value;
        int rank;        
    } in,out;

    in.rank=worldrank;
    in.value=estrazione;

    MPI_Allreduce(&in,&out,1,MPI_2INT,MPI_MAXLOC,MPI_COMM_WORLD);
    pivotrank=out.rank;
    printf("\nRango pivot: %d\n",pivotrank);

    int color=0;
    if(worldrank==pivotrank){ //pivot
        color=1;
    }

    MPI_Comm newcomm;
    MPI_Comm_split(MPI_COMM_WORLD,color,worldrank,&newcomm);

    if(color==1){ //pivot
        printf("P%d, sono pivot\n",worldrank);
        int A[k][p];
        FILE*f=fopen("mat.txt","r");
        for(int i=0; i<k; i++){
            for(int j=0; j<p; j++){
                fscanf(f,"%d",&A[i][j]);
            }
        }
        MPI_Datatype colsType;
        MPI_Type_vector(k,q,p,MPI_INT,&colsType);
        MPI_Type_commit(&colsType);

        int dests[nproc-1];
        for(int i=0; i<nproc; i++){
            if(i==worldrank) continue;
            dests[i]=i;
        }

        for(int i=0; i<nproc-1; i++){
            MPI_Send(&A[0][i*q],1,colsType,dests[i],0,MPI_COMM_WORLD);
        }

        int Bsum[k][q],Bprod[k][q];
        MPI_Status status;
        MPI_Recv(Bsum,k*q,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
        MPI_Recv(Bprod,k*q,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&status);

        printf("P%d(pivot), Bsum:\n",worldrank);
        for(int i=0; i<k; i++){
            for(int j=0; j<q; j++){
                printf("%d,",Bsum[i][j]);
            }
            printf("\n");
        }
        

        printf("P%d(pivot), Bprod:\n",worldrank);
        for(int i=0; i<k; i++){
            for(int j=0; j<q; j++){
                printf("%d.",Bprod[i][j]);
            }
            printf("\n");
        }
    }
    else{ //i processi non pivot
        printf("P%d, non sono pivot\n",worldrank);
        int B[k][q];
        MPI_Status status;
        MPI_Recv(B,k*q,MPI_INT,pivotrank,0,MPI_COMM_WORLD,&status);
        printf("P%d, B:\n",worldrank);
        for(int i=0; i<k; i++){
            for(int j=0; j<q; j++){
                printf("%d ",B[i][j]);
            }
            printf("\n");
        }

        MPI_Comm cart;
        int dims[2]={2,(nproc-1)/2};
        int periods[2]={1,1};

        MPI_Cart_create(newcomm,2,dims,periods,0,&cart);
        int cartrank;
        MPI_Comm_rank(cart,&cartrank);
        int coords[2];
        MPI_Cart_coords(cart,cartrank,2,coords);

        MPI_Comm rowcomm;
        MPI_Comm_split(cart,coords[0],cartrank,&rowcomm);
        int rowrank;
        MPI_Comm_rank(rowcomm,&rowrank);

        if(coords[0]==0){
            int Bsum[k][q];
            MPI_Reduce(B,Bsum,k*q,MPI_INT,MPI_SUM,0,rowcomm);
            if(rowrank==0){
                MPI_Send(Bsum,k*q,MPI_INT,pivotrank,0,MPI_COMM_WORLD);
            }
        }
        else{
            int Bprod[k][q];
            MPI_Reduce(B,Bprod,k*q,MPI_INT,MPI_PROD,0,rowcomm);
            if(rowrank==0){
                MPI_Send(Bprod,k*q,MPI_INT,pivotrank,1,MPI_COMM_WORLD);
            }
        }        
    }

    MPI_Finalize();
    
}