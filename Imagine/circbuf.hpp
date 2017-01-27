#ifndef CIRCBUF_HPP
#define CIRCBUF_HPP

/*
   the data is in [head tail). Let's say the capacity is 8.
   EG: empty: [0 0), [5 5)
       full: [0 8), [1 9), [2 10), [7 15)
       full-1: [0 7), [7 14)
   One way:
     head's range is [0 cap), so % is not used and cap doesn't have to be power of 2 to speed up %; also no overflow isue.
     so [8 x) ==> [0 x-8)
   The other way: head's range is [0, inf) so code is very simple.

   Alternatively, you can use the pair {head,size} to specify the range (tail=head+size;).
*/
class CircularBuf {
   int head,tail,cap; //we could add one more field "size" but it's not nec. But when use semaphore, a "size" field is used implicitly.
public:
   CircularBuf(int aCap) {head=tail=0; cap=aCap;}

   int size(){return tail-head;}
   int capacity() {return cap;}

   bool empty(){return size()==0; }
   bool full(){return size()==cap;}

   //remove from head. Return -1 when empty, otherwise return the just-dequed item's index in [0 cap) (that is, the old head).
   int get(){
      if(empty()) return -1;
      /* //when restrict head's value in [0 cap):
      int result=head;
      if(++head==cap) {head=0; tail-=cap;}
      return result;
      */
      return head++%cap;
   }//get(),

   //append at tail. Return -1 when full, otherwise return the just-enqued item's index in [0 cap) (that is, the old tail).
   int put(){
      if(full()) return -1;
      /* //when restrict head's value in [0 cap) so tail is in [0 2*cap):
      int result=tail>=cap?tail-cap:tail;
      tail++;
      return result;
      */
      return tail++%cap;
   }//put(),
};//class,

#endif //CIRCBUF_HPP
