#include <float.h>

#include <rsf.h>

#include "dijkstra.h"
#include "pqueue.h"

typedef enum {L, R, U, D} dir;

typedef struct Step {
    dir d;
    Step* next;
} *step;

static int n1, n2, np, **status;
static float **cost;
static step **path;
static const float big_value = FLT_MAX;

void dijkstra_init(int m1, int m2)
{
    int i2, i1;

    n1=m1;
    n2=m2;

    cost = sf_floatalloc2(n1,n2);
    status = sf_intalloc2(n1,n2);

    path = (step**) sf_alloc(n2,sizeof(step*));
    path[0] = (step*) sf_alloc(n1*n2,sizeof(step));
    for (i2=0; i2 < n2; i2++) {
	if (i2) path[i2] = path[0] + i2*n1;
	for (i1=0; i1 < n1; i1++) {
	    cost[i2][i1] = big_number;
	    status[i2][i1] = SF_OUT;
	    path[i2][i1] = NULL;
	}
    }
    sf_pqueue_init (2*(n1+n2));
}

void dijkstra_close(void)
{
    sf_pqueue_close ();
    free(cost[0]);
    free(cost);
    free(status[0]);
    free(status);
    free(path[0]);
    free(path);
}

static void fix_neighbor(int s1, int s2, int ud, int lr, float shift)
{
    float *neighbor, newcost, oldstatus;


    oldstatus = status[s2+lr][s1+ud];

    if (oldstatus == SF_IN) return;

    neighbor = cost[s2+lr]+s1+ud;
    newcost = cost[s2][s1] + shift;

    if (oldstatus == SF_OUT) {
	*neighbor = newcost;
	status[s2+lr][s1+ud] = SF_FRONT;
	sf_pqueue_insert (neighbor);
	np++;
    } else if (newcost < *neighbor) {
	*neighbor = newcost;
    }
}

static void neighbors(int s1, int s2, float **ud, float **lr)
{
    if (s1 < n1-1) { fix_neighbor(s1,s2,+1,0,ud[s2][s1  ]); }
    if (s1 > 0)    { fix_neighbor(s1,s2,-1,0,ud[s2][s1-1]); }
    if (s2 < n2-1) { fix_neighbor(s1,s2,0,+1,lr[s2  ][s1]); }
    if (s2 > 0)    { fix_neighbor(s1,s2,0,-1,lr[s2-1][s1]); }
}

void dijkstra(int s1, int s2, float **ud, float **lr) 
{
    int s;
    float *p;

    /* Intialize source */
    cost[s2][s1] = 0.;
    status[s2][s1] = SF_IN;
    sf_pqueue_start (); 
    np = 0.;
    neighbors(s1,s2,ud,lr);

    while (np > 0) {
	/* Extract smallest */
	p = sf_pqueue_extract(); 
	np--;

	if (p == NULL) {
	    sf_warning("%s: heap exausted!",__FILE__);
	    break;
	}

	s = p - cost[0];
	s2 = s/n1;
	s1 = s - s2*n1;

	status[s2][s1] = SF_IN;
	neighbors(s1,s2,ud,lr);
    }
}
    
