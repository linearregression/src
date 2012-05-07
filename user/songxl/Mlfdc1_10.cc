// 1D 10th-order Lowrank FD coefficient

//   Copyright (C) 2010 University of Texas at Austin
//  
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//  
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//  
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#include <time.h>

#include <rsf.hh>

#include "vecmatop.hh"
#include "serialize.hh"

using namespace std;

//FltNumVec vs; //c
//FltNumVec ks; //k
static std::valarray<float> vs, ks;
static float pi=SF_PI;
static float dt,dx;

int sample(vector<int>& rs, vector<int>& cs, FltNumMat& res)
{
    int nr = rs.size();
    int nc = cs.size();
    res.resize(nr,nc);  
    setvalue(res,0.0f);
    for(int a=0; a<nr; a++) {
	for(int b=0; b<nc; b++) {
        res(a,b) = 2.0*cos(2*pi*vs[rs[a]]*ks[cs[b]]*dt);
	}
    }
    return 0;
}

int FD10(vector<int>& rs, vector<int>& cs, FltNumMat& res)
{
    int nr = rs.size();
    int nc = cs.size();
    res.resize(nr,nc);  
    setvalue(res,0.0f);
    float dx2 = dx*dx;
    float b1  = 5.0/(3.0*dx2);
    float b2  = -5.0/(21.0*dx2);
    float b3  = 5.0/(126.0*dx2);
    float b4  = -5.0/(1008.0*dx2);
    float b5  = 1.0/(3150.0*dx2);
    float aa   = -2.0*(b1+b2+b3+b4+b5);
    float v, k;
    for(int a=0; a<nr; a++) {
	for(int b=0; b<nc; b++) {
           v = vs[rs[a]];
           k = ks[cs[b]]; 
           res(a,b) = v*v*dt*dt*(aa+2*b1*cos(2*pi*k*dx)+2.0*b2*cos(4*pi*k*dx)+2.0*b3*cos(6*pi*k*dx)+2.0*b4*cos(8*pi*k*dx)+2.0*b5*cos(10*pi*k*dx))+2.0;
	}
    }
    return 0;
}

int main(int argc, char** argv)
{   
    sf_init(argc,argv); // Initialize RSF

    iRSF par(0);
    int seed;

    par.get("seed",seed,time(NULL)); // seed for random number generator
    srand48(seed);

    float eps;
    par.get("eps",eps,1.e-4); // tolerance

    int npk;
    par.get("npk",npk,20); // maximum rank

    par.get("dt",dt); // time step

    iRSF velf;
    oRSF outm, Mexactfile("Mexact"), Mlrfile("Mlr"), Mappfile("Mapp"), Mfdfile("Mfd");
    //oRSF outm, Mexactfile("Mexact"), Mlrfile("Mlr");


    int N;
    velf.get("n1",N);
    float dk;
    velf.get("d1",dx);
    dk = 1.0/(dx*N);

    vs.resize(N);
    ks.resize(N);
    velf >> vs;
    
    int SIZE=6;
    outm.put("n1",N);
    outm.put("n2",SIZE);
    //outm.put("n2",N);
    int m = N;
    int n = N;

    int count = 0;
    float CUT = N/3.0*dk;
    for (int k=0; k < N; k++) {
	ks[k] = -dk*N/2.0+k*dk;
        if (abs(ks[k]) < CUT) count++;
    }
    vector<int> ksc(count);
    int nk=0;
    for (int k=0; k < N/2; k++) {
	ks[k] = k*dk;
        if (fabs(ks[k]) < CUT) {
           ksc[nk] = k;
           nk++;
        }
    }
    for (int k=N/2; k < N; k++) {
	ks[k] = (-N+k)*dk;
        if (fabs(ks[k]) < CUT) { 
           ksc[nk] = k;
           nk++;
        }
    } 
    
    vector<int> cidx, ridx;
    FltNumMat mid;
    iC( lowrank(m,n,sample,eps,npk,cidx,ridx,mid) );

    FltNumMat M1(m,cidx.size());
    vector<int> rs(m);
    for(int k=0; k<m; k++) rs[k]=k;
    iC( sample(rs,cidx,M1) );
    FltNumMat M2(ridx.size(),n);
    vector<int> cs(n);
    for(int k=0; k<n; k++) cs[k]=k;
    iC( sample(ridx,cs,M2) );
    FltNumMat Mexact(N,N);
    iC( sample(rs,cs,Mexact) );

    float dk2=dk/2.0;
    Mexactfile.put("n1",N);
    Mexactfile.put("n2",N);
    Mexactfile.put("d2",dk2);
    Mexactfile.put("o2",0);
    std::valarray<float> fMlr(N*N);
    float *ldat = Mexact.data();
    for (int k=0; k < N*N; k++) {
        fMlr[k] = ldat[k];
    } 
    Mexactfile << fMlr;

    Mlrfile.put("n1",N);
    Mlrfile.put("n2",N);
    Mlrfile.put("d2",dk2);
    Mlrfile.put("o2",0);
    Mappfile.put("n1",N);
    Mappfile.put("n2",N);
    Mappfile.put("d2",dk2);
    Mappfile.put("o2",0);
    Mfdfile.put("n1",N);
    Mfdfile.put("n2",N);
    Mfdfile.put("d2",dk2);
    Mfdfile.put("o2",0);

    FltNumMat Mlr(N,N);
    iC( FD10(rs,cs,Mlr) );
    ldat = Mlr.data();
    for (int k=0; k < N*N; k++) {
        fMlr[k] = ldat[k];
    } 
    Mfdfile << fMlr;

    cerr<<mid._m<<" ";
    cerr<<endl;
    cerr<<mid._n<<" ";
    cerr<<endl;
    cerr<<M2._m<<" ";
    cerr<<endl;
    cerr<<M2._n<<" ";
    cerr<<endl;
    FltNumMat tmpM(mid._m,M2._n);
    iC(dgemm(1.0,mid,M2,0.0,tmpM));
    iC(dgemm(1.0,M1,tmpM,0.0,Mlr));
    ldat = Mlr.data();
    for (int k=0; k < N*N; k++) {
        fMlr[k] = ldat[k];
    } 
    Mlrfile << fMlr;
/*  Next */

    float stmp[] = {0,1,2,3,4,5};
    FltNumMat s(SIZE,1); for(int k=0; k<SIZE; k++) s._data[k]=stmp[k];    
    FltNumMat ktmp(1,N); for(int k=0; k<N; k++) ktmp._data[k]=ks[k];
    FltNumMat ktmpc(1,count); for(int k=0; k<count; k++) ktmpc._data[k]=ks[ksc[k]];
    FltNumMat B(SIZE,N), Bc(SIZE,count);
    iC(dgemm(2*pi*dx,s,ktmp,0.0,B));
    iC(dgemm(2*pi*dx,s,ktmpc,0.0,Bc));
    for(int k=0; k<B._m*B._n; k++) B._data[k]=cos(B._data[k]);
    for(int k=0; k<Bc._m*Bc._n; k++) Bc._data[k]=cos(Bc._data[k]);
    FltNumMat IB(N,SIZE);    iC( pinv(B, 1e-16, IB) );
    FltNumMat IBc(count,SIZE);    iC( pinv(Bc, 1e-16, IBc) );
    FltNumMat coef(ridx.size(),SIZE);
    FltNumMat M2c;
    iC( sample(ridx,ksc,M2c) );
    
    iC(dgemm(1.0,M2c,IBc,0.0,coef));

    FltNumMat G(N,SIZE), tmpG(mid._m,SIZE);
    iC(dgemm(1.0,mid,coef,0.0,tmpG));
    iC(dgemm(1.0,M1,tmpG,0.0,G));

    Bc.resize(SIZE,1);
    for(int k=0; k<SIZE; k++) Bc._data[k]=B(k,0);
    FltNumMat tmpB(N,1);
    iC(dgemm(1.0,G,Bc,0.0,tmpB));
    for(int k=0; k<N; k++) tmpB._data[k]=2.0/tmpB._data[k];
    sf_warning("eeeee");
    for (int x=0; x<N; x++){
        for (int k=0; k<SIZE; k++){
            G(x,k) = G(x,k)*tmpB._data[x];
        }
    }
    sf_warning("bbbb");

      
    iC(dgemm(1.0,G,B,0.0,Mlr));
    ldat = Mlr.data();
    for (int k=0; k < N*N; k++) {
        fMlr[k] = ldat[k];
    } 
    sf_warning("cccc");
    Mappfile << fMlr;

    ldat = G.data();
    fMlr.resize(N*SIZE);
    sf_warning("dddd");
    for (int k=0; k < SIZE*N; k++) {
        fMlr[k] = ldat[k];
    } 
    outm << fMlr;
  //  */
    return 0;
}





