

//#include "stdafx.h"
//#include "Trace.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "htable.h"
#include "util.h"
// #include "edelta.h"
/*
 * Take each hash link and walk down the chain of items
 *  that hash there counting them (i.e. the hits), 
 *  then report that number.
 *  Obiously, the more hits in a chain, the more time
 *  it takes to reference them. Empty chains are not so
 *  hot either -- as it means unused or wasted space.
 */
#define MAX_COUNT 20
#define STRLOOK 16   //fix size

/* ===================================================================
 *    htable
 */

/*
 * Create hash of key, stored in hash then
 *  create and return the pseudo random bucket index
 */
 
#define HASH_SIZE 40 
#define UINT32_SIZE 4 


void htable::hash_index(unsigned char *key)
{ 
   //hash = 0;   
   //u_int64_t nKey=0;
  
   if(hashlength==8 && key)
   {
   		memcpy((char*)&hash,key,hashlength);
		index = hash & mask;
		return;
   		//nkey = ;
   		//index=hash;
   		//index = ((hash * 1103515249) >> rshift) & mask;
   }
   if(key==NULL)
   {
   		// printf("%s,%d:Insert key==NULL\n",__FILE__,__LINE__);
		index=0;
   		return ;
   }
   for (int i = hashlength-1; hash < buckets && i > 0; i--) {
      hash += (hash *16) + (key[i]<'9'?key[i]-'0':10+key[i]-'A');
   }
   ///Multiply by large prime number, take top bits, mask for remainder 
   index = hash & mask;

   return;
}

htable::htable(int offset, int tsize)
{
   init(offset, tsize);
}

/* @tsize: num of max_items before growing ,so
* tsize=max_items=buckets * 4
*
* @nhlen: hashlength in bytes
*/
void htable::init(int offset,int nhlen, int tsize)
{
   int pwr;
   tsize >>= 2;//because: max_items = buckets * 4;
   for (pwr=0; tsize; pwr++) {
      tsize >>= 1;
   }
   loffset = offset;
   mask = ~((~0)<<pwr);               /* 3 bits => table size = 8 */
   rshift = 30 - pwr;                 /* start using bits 28, 29, 30 */
   num_items = 0;                     /* number of entries in table */
   buckets = 1<<pwr;                  /* hash table size -- power of two */
   max_items = buckets * 4;           /* allow average 4 entries per chain */
   table = (hlink **)malloc(buckets * sizeof(hlink *));
   memset(table, 0, buckets * sizeof(hlink *));
   walkptr = NULL;
   walk_index = 0;
   hashlength = nhlen;
}
 void *htable::lookup1(unsigned char *key,float pos)
{
	 int i=  0;
	    index = (*(uint64_t*) key* 2654435761U) & mask;
	        void *point =NULL;
		    float distance=1;
//		    printf("======================\n");
		    hlink *pre=NULL;
		        for (hlink *hp=table[index]; hp;pre=hp,hp=(hlink *)hp->next)
				    {            
						



					           i++; 
					            if(*(uint64_t *)(key)  ==  *(uint64_t *)( hp->key))
							            { //	 printf("i:%d\n",i);

									    if(pre==NULL) table[index]=(hlink *)hp->next;
									    else pre->next=(hlink *)hp->next;
										//printf("dis:%.9f\n",fabs(hp->position-pos));						
									   // printf("hash:%ld\n",*(uint64_t *)( hp->key));
									                if(fabs(hp->position-pos)<distance)
												            {
														                    point=((char *)hp)-loffset;
																                    distance=fabs(hp->position-pos);
																		                }
								//	if(i==1) break;	
											    // break;  
											        }
						     
//	break;
			
    				    }
			 
		//	printf("dis:%.9f\n",distance);	
	//	printf("i:%d\n",i);
	//	printf("======================\n");


			    return point;
}

 bool htable::insert1(unsigned char *key, void *item,float pos)
{
	    hlink *hp;

	        index = (*(uint64_t*) key* 2654435761U) & mask;       

		    hp = (hlink *)(((char *)item)+loffset);
		        hp->next = table[index]; 
			    hp->position=pos;

			        hp->key = key;
				    table[index] = hp;

				        if (++num_items >= max_items) {
						        grow_table();
							    }
					    return true;
}



u_int32_t htable::size()
{
   return num_items;
}

void htable::stats()
{
   int hits[MAX_COUNT];
   int max = 0;
   int i, j;
   hlink *p;
   // printf("\n\nNumItems=%d\nTotal buckets=%d\n", num_items, buckets);
   // printf("Hits/bucket: buckets\n");
   for (i=0; i < MAX_COUNT; i++) {
      hits[i] = 0;
   }
   for (i=0; i<(int)buckets; i++) {
      p = table[i];
      j = 0;
      while (p) {
         p = (hlink *)(p->next);
         j++;
      }
      if (j > max) {
         max = j;
      }
      if (j < MAX_COUNT) {
         hits[j]++;
      }
   }
   for (i=0; i < MAX_COUNT; i++) {
      // printf("%2d:           %d\n",i, hits[i]);
   }
   // printf("max hits in a bucket = %d\n", max);
}

void htable::grow_table()
{
   //Dmsg1(100, "Grow called old size = %d\n", buckets);
   /* Setup a bigger table */
   htable *big = (htable *)malloc(sizeof(htable));
   big->loffset = loffset;
   big->hashlength  = hashlength;
   big->mask = mask<<1 | 1;
   big->rshift = rshift - 1;
   big->num_items = 0;
   big->buckets = buckets * 2;
   big->max_items = big->buckets * 4;
   big->table = (hlink **)malloc(big->buckets * sizeof(hlink *));
   memset(big->table, 0, big->buckets * sizeof(hlink *));
   big->walkptr = NULL;
   big->walk_index = 0;
   /* Insert all the items in the new hash table */
   //Dmsg1(100, "Before copy num_items=%d\n", num_items);
   /*
    * We walk through the old smaller tree getting items,
    * but since we are overwriting the colision links, we must
    * explicitly save the item->next pointer and walk each
    * colision chain ourselves.  We do use next() for getting
    * to the next bucket.
    */
   for (void *item=first(); item; ) {
      void *ni = ((hlink *)((char *)item+loffset))->next;  /* save link overwritten by insert */
      //Dmsg1(100, "Grow insert: %s\n", ((hlink *)((char *)item+loffset))->key);
      big->insert(((hlink *)((unsigned char *)item+loffset))->key, item);
      if (ni) {
         item = (void *)((char *)ni-loffset);
      } else {
         walkptr = NULL;
         item = next();
      }
   }
   //Dmsg1(100, "After copy new num_items=%d\n", big->num_items);
   if (num_items != big->num_items) {
      //Dmsg0(000, "****** Big problems num_items mismatch ******\n");
   }
   free(table);
   memcpy(this, big, sizeof(htable));  /* move everything across */
   free(big);
   //Dmsg0(100, "Exit grow.\n");
}

bool htable::insert(unsigned char *key, void *item)
{
   hlink *hp;

   /*if(key==NULL)
   {
   		printf("%s,%d:Insert key==NULL\n",__FILE__,__LINE__);
   		return false;
   }*/
   //hash_index(key);
   index = (*(uint64_t*) key* 2654435761U) & mask;
   
   hp = (hlink *)(((char *)item)+loffset);
   hp->next = table[index]; /*some pro*/
   
   //hp->hash = hash;
   //memcpy(&hp->hash,&hash,sizeof(hash));
   //TRACE("%d",sizeof(hash));
   //free(item);
   hp->key = key;
   table[index] = hp;

   if (++num_items >= max_items) {
      //printf("%s,%d:grow_table_hash num_items=%d max_items=%d\n", __FILE__,__LINE__,num_items, max_items);
      grow_table();
   }
   return true;
}

void *htable::lookup(unsigned char *key)
{
   //hash_index(key);
   index = (*(uint64_t*) key* 2654435761U) & mask;

   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next)
   {
      //if (memcmp(key, hp->key,hashlength) == 0) //hash == hp->hash && 
     if(*(uint64_t *)(key)  ==  *(uint64_t *)( hp->key))
     {
         //Dmsg1(100, "lookup return %x\n", ((char *)hp)-loffset);
         return ((char *)hp)-loffset;
      }
   }
   return NULL;
}


#define SESIZE 10
void *htable::lookup_fix(unsigned char *key, unsigned char *New, unsigned char *Base,uint32_t baselength,uint32_t newlength)
{

    index = (*(uint64_t*) key* 2654435761U) & mask;
    int count=0;
    uint32_t bestnum =0;
    hlink* best =NULL;
    int time =0;
    for (hlink *hp=table[index]; hp && count<SESIZE  && time <800; hp=(hlink *)hp->next,time++)
    {

        if(*(uint64_t *)(key)  ==  *(uint64_t *)( hp->key))
        {
            if(memcmp( New ,Base + ((DeltaRecord *)(((char *)hp)-loffset))->nOffset,STRLOOK)==0 )
            {

                uint32_t j=STRLOOK;
                while( ((DeltaRecord *)(((char *)hp)-loffset))->nOffset+STRLOOK+j < baselength &&
                        STRLOOK+j < newlength )
                {
                    if(Base[((DeltaRecord *)(((char *)hp)-loffset))->nOffset+STRLOOK+j] == New[STRLOOK+j]){
                        j++;
                    }
                    else
                        break;
                }
                if(j>bestnum)
                {
//                    count ++;
                    if(bestnum!=0)
                    {
//                        printf("pre=%d\n",bestnum);
//                        printf("next=%d\n",j);
                    }
                    bestnum=j;
                    best =hp;
                }
                count ++;

            }

        }

    }
//    printf("search time;%d\n",time);
    if(bestnum == 0) return NULL;
    else  return ((char *)best)-loffset;

}
void *htable::search(unsigned char *key)
{
   //hash_index(key);
   index = (*(uint64_t*) key* 2654435761U) & mask;
   for (hlink *hp=table[index]; hp; hp=(hlink *)hp->next) 
   {
      //if (memcmp(key, hp->key,hashlength) == 0) //hash == hp->hash && 
      if(*(uint64_t *)(key)  ==  *(uint64_t *)( hp->key))
	{
               //Dmsg1(100, "lookup return %x\n", ((char *)hp)-loffset);
		 return hp;
      }

  }
  return NULL;

}

void *htable::next()
{
   //Dmsg1(100, "Enter next: walkptr=0x%x\n", (unsigned)walkptr);
   if (walkptr) {
      walkptr = (hlink *)(walkptr->next);
   }
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];
      if (walkptr) {
         //Dmsg3(100, "new walkptr=0x%x next=0x%x inx=%d\n", (unsigned)walkptr,(unsigned)(walkptr->next), walk_index-1);
      }
   }
   if (walkptr) {
      //Dmsg2(100, "next: rtn 0x%x walk_index=%d\n",(unsigned)(((char *)walkptr)-loffset), walk_index);
      return ((char *)walkptr)-loffset;
   }
   //Dmsg0(100, "next: return NULL\n");
   return NULL;
}

void *htable::first()
{
   //Dmsg0(100, "Enter first\n");
   walkptr = table[0];                /* get first bucket */
   walk_index = 1;                    /* Point to next index */
   while (!walkptr && walk_index < buckets) {
      walkptr = table[walk_index++];  /* go to next bucket */
      if (walkptr) {
         //Dmsg3(100, "first new walkptr=0x%x next=0x%x inx=%d\n", (unsigned)walkptr,(unsigned)(walkptr->next), walk_index-1);
      }
   }
   if (walkptr) {
      //Dmsg1(100, "Leave first walkptr=0x%x\n", (unsigned)walkptr);
      return ((char *)walkptr)-loffset;
   }
   //Dmsg0(100, "Leave first walkptr=NULL\n");
   return NULL;
}

/* Destroy the table and its contents */
void htable::destroy()
{
   void *ni;
   void *li = first();
   do {
      ni = next();
      free(li);
      li = ni;
   } while (ni);

   free(table);
   table = NULL;
   //Dmsg0(100, "Done destroy.\n");
}

void htable::SetOffset(int offset)
{
   loffset = offset;
   return ;
}

void htable::ResetTable()
{
   memset(table, 0, buckets * sizeof(hlink *));
   num_items = 0;
   return ;
}