// extbtree.cpp
#ifdef WINS
#include <windows.h>
#else
#include "winrepl.h"
#endif
#include "sdefs.h"
#include "scomp.h"
#include "basic.h"
#include "frame.h"
#include "kurzor.h"
#include "dispatch.h"
#include "table.h"
#include "dheap.h"
#include "curcon.h"
#pragma hdrstop
#ifdef UNIX
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#endif

#include "extbtree.h"

// implementation of the clustered btree index

int allocated_cb_frames = 0; 

cbnode_ptr alloc_cb_frame(void)
{ allocated_cb_frames++;
  char * ptr = (char*)malloc(cb_cluster_size+2); 
#ifdef _DEBUG
  *ptr=13;  ptr[1+cb_cluster_size]=0x7c;
#endif
  return (cbnode_ptr)(ptr+1);
}

void free_cb_frame(cbnode_ptr frame)
{ allocated_cb_frames--;
  char * ptr = (char*)frame - 1;
#ifdef _DEBUG
  if (*ptr!=13 || ptr[1+cb_cluster_size]!=0x7c)
    frame=frame;
#endif
  free(ptr); 
}

///////////////////////// path //////////////////////////////////
cbnode_path_item * cbnode_path::push(ext_index_file & eif, cbnode_number num, cbnode_oper & oper)
{
  if (top >= MAX_CBNODE_PATH-1)
    { eif.err_stack++;  return NULL; }
  cbnode_path_item * curr_path_item = &items[++top];
  curr_path_item->ptr=alloc_cb_frame();
  if (!curr_path_item->ptr) 
    { eif.err_alloc++;  return NULL; }
  curr_path_item->num = num;
  if (!eif.read(num, curr_path_item->ptr, oper))
    { eif.err_read++;  return NULL; }
  return curr_path_item;
}

void cbnode_path::pop(void)
// [top]>=0 is supposed on entry!
{ 
  free_cb_frame(items[top--].ptr);
}

void cbnode_path::close(void)
{
  while (top>=0)
    pop();
}

void cbnode_path::check(void)
{
  for (int i=0;  i<=top;  i++)
  { char * ptr = (char*)items[i].ptr - 1;
    if (*ptr!=13 || ptr[1+cb_cluster_size]!=0x7c)
      ptr=ptr;
  }
}

void * cbnode_path::clustered_value_ptr(cbnode_oper & oper)
{
  cbnode_ptr cbnode = items[top].ptr;
  return cbnode->key(items[top].pos, cbnode->is_leaf() ? oper.leaf_step : oper.inner_step) + oper.key_value_size;
}

int cbnode_oper_wi::cmp_keys(const void * key1, const void * key2)
{
  return strncmp((const char *)key1, (const char *)key2, MAX_FT_WORD_LEN);
}

cbnode_oper_wi::cbnode_oper_wi(void)
{
  key_value_size=MAX_FT_WORD_LEN;
  clustered_value_size=sizeof(t_wordid);
  define_steps();
}

int cbnode_oper_ri1_32::cmp_keys(const void * key1, const void * key2)
{
  unsigned * ki1 = (unsigned*)key1;
  unsigned * ki2 = (unsigned*)key2;
  if (*(t_wordid*)key1 != *(t_wordid*)key2)
    return *(t_wordid*)key1 < *(t_wordid*)key2 ? -1 : 1;
  key1 = (char*)key1 + sizeof(t_wordid);
  key2 = (char*)key2 + sizeof(t_wordid);
  if (*(int*)key1 != *(int*)key2)  // NONEINTEGER is used and must be the smallest value
    return *(int*)key1 < *(int*)key2 ? -1 : 1;
  key1 = (char*)key1 + sizeof(uns32);
  key2 = (char*)key2 + sizeof(uns32);
  if (*(unsigned*)key1 != *(unsigned*)key2)
    return *(unsigned*)key1 < *(unsigned*)key2 ? -1 : 1;
  return 0;
}

cbnode_oper_ri1_32::cbnode_oper_ri1_32(void)
{
  key_value_size=sizeof(t_wordid)+sizeof(uns32)+sizeof(unsigned);
  clustered_value_size=sizeof(char);
  define_steps();
}

int cbnode_oper_ri1_64::cmp_keys(const void * key1, const void * key2)
{
  unsigned * ki1 = (unsigned*)key1;
  unsigned * ki2 = (unsigned*)key2;
  if (*(t_wordid*)key1 != *(t_wordid*)key2)
    return *(t_wordid*)key1 < *(t_wordid*)key2 ? -1 : 1;
  key1 = (char*)key1 + sizeof(t_wordid);
  key2 = (char*)key2 + sizeof(t_wordid);
  if (*(sig64*)key1 != *(sig64*)key2)  // NONEBIGINT is used and must be the smallest value  ??
    return *(sig64*)key1 < *(sig64*)key2 ? -1 : 1;
  key1 = (char*)key1 + sizeof(t_docid);
  key2 = (char*)key2 + sizeof(t_docid);
  if (*(unsigned*)key1 != *(unsigned*)key2)
    return *(unsigned*)key1 < *(unsigned*)key2 ? -1 : 1;
  return 0;
}

cbnode_oper_ri1_64::cbnode_oper_ri1_64(void)
{
  key_value_size=sizeof(t_wordid)+sizeof(t_docid)+sizeof(unsigned);
  clustered_value_size=sizeof(char);
  define_steps();
}

int cbnode_oper_ri2_32::cmp_keys(const void * key1, const void * key2)
{
  return *(uns32*)key1<*(uns32*)key2 ? -1 : *(uns32*)key1==*(uns32*)key2 ? 0 : 1;
}

cbnode_oper_ri2_32::cbnode_oper_ri2_32(void)
{
  key_value_size=sizeof(uns32);
  clustered_value_size=sizeof(t_wordid);
  define_steps();
}

int cbnode_oper_ri2_64::cmp_keys(const void * key1, const void * key2)
{
  return *(t_docid*)key1<*(t_docid*)key2 ? -1 : *(t_docid*)key1==*(t_docid*)key2 ? 0 : 1;
}

cbnode_oper_ri2_64::cbnode_oper_ri2_64(void)
{
  key_value_size=sizeof(t_docid);
  clustered_value_size=sizeof(t_wordid);
  define_steps();
}

void cbnode_oper::define_steps(void)
{ leaf_step =key_value_size+clustered_value_size;
  inner_step=key_value_size+sizeof(cbnode_number);
}

cb_result cbnode_oper::search_node(const void * key, cbnode_ptr cbnode, cbnode_pos & pos)
// Searches the given key in the given node. 
// Returns: CB_NOT_FOUND & pos==-1 when the node is empty or when the key is less than the 1st value in the node.
//          CB_FOUND_EXACT when the key has been found in [pos]
//          CB_FOUND_CLOSEST when the key has not been found and [pos] is the biggest value less than [key].
// [pos] is either -1 or position of existing key - never points after the last key.
{ unsigned step;  int res;  unsigned lind, rind, mind;  
  if (cbnode->is_leaf())
    step=leaf_step;
  else
    step=inner_step;
  rind = (cbnode->used_space - sizeof(cb_node)) / step;
  lind=0;
  if (!rind) { pos=-1;  return CB_NOT_FOUND; } /* node empty */
  /* invariant: searching <lind, rind) interval */
  while (lind+1 < rind)  /* more than 1 key in the interval */
  { mind=(lind+rind)/2;
    res=cmp_keys(key, cbnode->key(mind, step));
    if (!res) // found
      { pos=mind;  return CB_FOUND_EXACT; }
    if (res > 0)
      lind=mind;
    else
      rind=mind;
  }
 // lind or nothing:
  res=cmp_keys(key, cbnode->key(lind, step));
  if (res<0)
    { pos=-1;  return CB_NOT_FOUND; } // before lind -> before the 1st value in the node
  pos=lind;
  return res==0 ? CB_FOUND_EXACT : CB_FOUND_CLOSEST;
}

cb_result cb_build_stack(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value, cbnode_path & path)
// On entry: [root] exists
// On exit: path.top>=0, path.items[path.top] is a leaf, [path] is the path.
// Inner elements of path: when pos==-1, path goes to the leftmost son, otherwise goes to the pos-th son.
{ cb_result res;  cbnode_path_item * curr_path_item;
  cbnode_number current_node = root;
  path.top=-1;
  do
  {// open the current_node and add it to the path:
    curr_path_item = path.push(eif, current_node, oper);
    if (!curr_path_item)
      return CB_ERROR;
   // search the current node:
    res = oper.search_node(key_value, curr_path_item->ptr, curr_path_item->pos);
    if (curr_path_item->ptr->is_leaf()) break;
   // get the number of the proper son:
    if (res==CB_NOT_FOUND) current_node = curr_path_item->ptr->leftmost_son;
    else current_node = *curr_path_item->ptr->son(curr_path_item->pos, oper.inner_step);  // same or less value found, but the next value is bigger
  } while (true);
  return res;
}

cb_result cb_build_stack_find(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value, cbnode_path & path)
{ 
  cb_result res = cb_build_stack(eif, oper, root, key_value, path);
 // have the leaf node:

  if (res==CB_FOUND_CLOSEST || res==CB_NOT_FOUND)  
  { cbnode_path_item * curr_path_item = &path.items[path.top];
    int cmpres;  uns8 new_key_value[MAX_CBKEY_LENGTH];
    do
    { if (!cb_step(eif, oper, new_key_value, path))
        return CB_NOT_FOUND;
      cmpres=oper.cmp_keys(new_key_value, key_value);
    } while (cmpres<0);
    if (!cmpres) res=CB_FOUND_EXACT;
    else res=CB_FOUND_CLOSEST;
  }
  return res;
}

bool cb_step(ext_index_file & eif, cbnode_oper & oper, void * key_value, cbnode_path & path)
// Goes to the next value and copies it to the [key_value]. Returns false when no next value exists.
{ cbnode_path_item * curr_path_item;  unsigned step, rind;
 // go up, while no next pos exists:
  do
  { curr_path_item = &path.items[path.top];
    if (curr_path_item->ptr->is_leaf())
      step=oper.leaf_step;
    else
      step=oper.inner_step;
    rind = (curr_path_item->ptr->used_space - sizeof(cb_node)) / step;
    if (curr_path_item->pos+1<rind) break;
    path.pop();
    if (path.top<0) return false;  // end of tree
  } while (true);
  curr_path_item->pos++;
 // go to [pos] and down to the leftmost leaf:
  if (!curr_path_item->ptr->is_leaf())
    do
    { cbnode_number new_num = *curr_path_item->ptr->son(curr_path_item->pos, oper.inner_step);
      curr_path_item = path.push(eif, new_num, oper);
      if (!curr_path_item) 
        return false;
      if (curr_path_item->ptr->is_leaf())
        { curr_path_item->pos=0;  break; }
      curr_path_item->pos=-1;  // will go to the leftmost son
    } while (true);
 // copy the key value:
  memcpy(key_value, curr_path_item->ptr->key(curr_path_item->pos, oper.leaf_step), oper.key_value_size);
  return true;
}

static bool verify_ordering(void * & last_key_value, void * key_value, cbnode_oper & oper)
{ int res;
  if (last_key_value)
    res=oper.cmp_keys(last_key_value, key_value);
  else
  { last_key_value = malloc(oper.key_value_size);
    res=-1;
  }
  if (res<0) memcpy(last_key_value, key_value, oper.key_value_size);
  return res<=0;
}

void cb_pass_all(ext_index_file & eif, cbnode_oper & oper, cbnode_number root)
{ cbnode_path path;  cbnode_path_item * curr_path_item;
  void * last_key_value = NULL;
  path.top=-1;
  curr_path_item = path.push(eif, root, oper);
  if (curr_path_item) 
  { curr_path_item->pos=-1;
    do // AXIOM: either top is leaf or top/pos if the node to be processed (pos inside path point to the next node to be processed)
    { curr_path_item = &path.items[path.top];
      if (curr_path_item->ptr->is_leaf())
      {// verify the ordering:
        int rind = (curr_path_item->ptr->used_space - sizeof(cb_node)) / oper.leaf_step;
        for (int i=0;  i<rind;  i++)
          if (!verify_ordering(last_key_value, curr_path_item->ptr->key(i, oper.leaf_step), oper))
            eif.err_order++;
       // remove the processed leaf:
        path.pop();
      }
      else  // inner node
      {// check if the is any unprocessed son of the node
        unsigned rind = (curr_path_item->ptr->used_space - sizeof(cb_node)) / oper.inner_step;
        if (curr_path_item->pos+1<rind) 
        {// verify the ordering: 
          if (curr_path_item->pos>=0)
            if (!verify_ordering(last_key_value, curr_path_item->ptr->key(curr_path_item->pos, oper.inner_step), oper))
              eif.err_order++;
         // go down:
          curr_path_item = path.push(eif, *curr_path_item->ptr->son(curr_path_item->pos++, oper.inner_step), oper);
          if (!curr_path_item) 
            { path.pop();  curr_path_item = &path.items[path.top]; }
          curr_path_item->pos=-1;
        }
        else   
          path.pop();
      }
    } while (path.top>=0);
    free(last_key_value);
  }
}

void cbnode_oper::insert_direct(cbnode_ptr ptr, cbnode_pos pos, void * key_value, void * clustered_value, unsigned step)
{ pos++;  // convert pre-pos to insert-pos:
 // make space:
  memmove(ptr->key(pos+1, step), ptr->key(pos, step), ptr->used_space - sizeof(cb_node) - step * pos);
  ptr->used_space+=step;
 // copy the value: key and either the clustered value or the node number:
  memcpy(ptr->key(pos, step), key_value, key_value_size);
  memcpy(ptr->key(pos, step)+key_value_size, clustered_value, ptr->is_leaf() ? clustered_value_size : sizeof(cbnode_number));
}

cb_result insert_1(ext_index_file & eif, cbnode_oper & oper, cbnode_path_item & item, void * key_value, void * clustered_value, void * split_value, cbnode_number * split_node_num)
// Writes the updated node(s).
// Does not free the item.ptr, but frees the new node, if created.
{ unsigned step;  
  if (item.ptr->is_leaf())
    step=oper.leaf_step;
  else
    step=oper.inner_step;
  if (item.ptr->used_space+step > cb_cluster_size)
  {// must split:
    unsigned count = (item.ptr->used_space - sizeof(cb_node)) / step;
    unsigned count1 = count/2, count2 = count-count1;
    cbnode_ptr new_node = alloc_cb_frame();
    new_node->magic = item.ptr->magic;
    if (item.ptr->is_leaf())
    {// left node (original):
      item.ptr->used_space -= count2*step;
     // right node (new):
      new_node->used_space = (unsigned)sizeof(cb_node) + step * count2;
      memcpy(new_node->key(0, step), item.ptr->key(count1, step), step * count2);
     // insert (AFTER item.pos):
      if (item.pos < (int)count1)
        oper.insert_direct(item.ptr, item.pos, key_value, clustered_value, step);
      else
        oper.insert_direct(new_node, item.pos-count1, key_value, clustered_value, step);
     // define split value:
      memcpy(split_value, new_node->key(0, step), oper.key_value_size);
    }
    else  // splitting the inner node: 1 key value will be removed and copied upwards
    { if (count1<count2) { count1++;  count2--; }
      count1--;  // 1 value will be removed
     // left node (original):
      item.ptr->used_space = (unsigned)sizeof(cb_node) + step * count1;
     // right node (new):
      new_node->used_space = (unsigned)sizeof(cb_node) + step * count2;
      memcpy(new_node->key(0, step), item.ptr->key(count1+1, step), step * count2);
      new_node->leftmost_son = *item.ptr->son(count1, step);
     // define split value:
      uns8 new_split_value[MAX_CBKEY_LENGTH];
      memcpy(new_split_value, item.ptr->key(count1, step), oper.key_value_size);
     // insert: (probably it is not necessary to compare and search again)
      int res=oper.cmp_keys(key_value, new_split_value);  // comparing to the value moved above
      if (res<0)
        oper.insert_direct(item.ptr, item.pos, key_value, clustered_value, step);
      else
      { cbnode_pos npos;
        oper.search_node(key_value, new_node, npos);
        oper.insert_direct(new_node, npos, key_value, clustered_value, step);
      }
      memcpy(split_value, new_split_value, oper.key_value_size);  // may be split_value==key_value, so must not overwrite split_value before writing key_value!
    }
   // write node(s):
    eif.write(item.num, item.ptr);
    *split_node_num = eif.alloc_cbnode();
    eif.write(*split_node_num, new_node);
    free_cb_frame(new_node);
    return CB_SPLIT;
  }
  else  // insert into the [pos]+1 position
  { oper.insert_direct(item.ptr, item.pos, key_value, clustered_value, step);
   // write changes:
    return eif.write(item.num, item.ptr) ? CB_FOUND_EXACT : CB_ERROR;
  }
}

bool cb_insert_val(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value, void * clustered_value)
// Called with the write lock on [eif].
{
  cbnode_path path;  uns8 split_value[MAX_CBKEY_LENGTH];  cbnode_number split_node_num;
  cb_result res = cb_build_stack(eif, oper, root, key_value, path);
  if (res==CB_ERROR) return false;
  do
  { res=insert_1(eif, oper, path.items[path.top], key_value, clustered_value, split_value, &split_node_num);
    if (res==CB_ERROR) return false;
    if (res!=CB_SPLIT) break;
   // prepare another inserting:
    path.pop();
    key_value=split_value;
    clustered_value=&split_node_num;
   // create a new root, if necessary:
    if (path.top<0)
    { cbnode_ptr new_node = alloc_cb_frame();
      cbnode_number new_node_num = eif.alloc_cbnode();
      if (!eif.read(root, new_node, oper)) return CB_ERROR;  // reads the original root contents to new_node
      if (!eif.write(new_node_num, new_node)) return CB_ERROR;  // saves the original root contents to new_node_num
     // create the new root
      new_node->magic = cbnode_inner_magic;
      new_node->leftmost_son = new_node_num;
      memcpy(new_node->key(0, oper.inner_step), split_value, oper.key_value_size);
      *new_node->son(0, oper.inner_step) = split_node_num;
      new_node->used_space = (unsigned)sizeof(cb_node) + oper.inner_step;
     // save the new root contents:
      if (!eif.write(root, new_node)) return CB_ERROR;  // saves the original root contents to new_node_num
      free_cb_frame(new_node);
      break;
    }
  } while (true);
  return true;
}

#if 0
bool cb_remove_val(ext_index_file & eif, cbnode_oper & oper, cbnode_number root, void * key_value)
// Never called
{
  t_writer mrsw_writer(&eif.mrsw_lock, cdp);
  cbnode_path path;  
  cb_result res = cb_build_stack(eif, oper, root, key_value, path);
  if (res==CB_ERROR) return false;
  if (res!=CB_FOUND_EXACT) 
  { dbg_err("External btree node not found");
    return true;  // node not found
  }
  return _cb_remove_val(eif, oper, path);
}
#endif

bool _cb_remove_val(ext_index_file & eif, cbnode_oper & oper, cbnode_path & path)
// Called with the write lock on [eif].
{ bool last_join_was_left=false;
  do
  { bool joined=false;  
    unsigned step;  
    cbnode_ptr curr_node = path.items[path.top].ptr;
    unsigned   curr_pos  = path.items[path.top].pos;
    if (curr_node->is_leaf())  // removing the leaf node at [pos]
    { step=oper.leaf_step;
      memmove(curr_node->key(curr_pos, step), curr_node->key(curr_pos+1, step), curr_node->used_space - sizeof(cb_node) - step * (curr_pos+1));
      curr_node->used_space-=step;
      if (curr_node->used_space == sizeof(cb_node))  // minimal leaf node, will disppear
      { if (path.top>0) 
        { last_join_was_left = path.items[path.top-1].pos > -1;  // last_join_was_left must be defined
         // deallocate and unlock the path node, make the path shorter:
          eif.free_cbnode(path.items[path.top].num);
          path.pop();
         // NOTE: not writing [curr_node].
          continue;
        }
        // root node must be saved even if it is empty
      }
    }
    else 
    { step=oper.inner_step;
      if (curr_node->used_space == sizeof(cb_node))  // minimal inner node, will disppear
      { if (path.top>0) last_join_was_left = path.items[path.top-1].pos > -1;  // last_join_was_left must be defined
       // deallocate and unlock the path node, make the path shorter:
        eif.free_cbnode(path.items[path.top].num);
        path.pop();
       // NOTE: not writing [curr_node].
        continue;
      }
      else if (last_join_was_left)  // pos > -1
        memmove(curr_node->key(curr_pos, step), curr_node->key(curr_pos+1, step), curr_node->used_space - sizeof(cb_node) - step * (curr_pos+1));
      else
        memmove(curr_node->son(curr_pos, step), curr_node->son(curr_pos+1, step), curr_node->used_space - sizeof(cb_node) - step * (curr_pos+1) - oper.key_value_size);
      curr_node->used_space-=step;
    }
   // check for joining the node with its neighbourghs:
   // RULE: when joining nodes, the node in the path will disappear, no matter if joining with the left or right brother.
    unsigned local_data = curr_node->used_space - sizeof(cb_node);  // size of values, does not include the header
    if (local_data > cb_cluster_size/2) 
    { eif.write(path.items[path.top].num, curr_node);
      break;
    }
   // processing the node with the small contents:
    if (path.top>0)
    { cbnode_ptr brother_node = alloc_cb_frame();
      cbnode_number brother_num;  
     // first, try the left brother:
      if (path.items[path.top-1].pos > -1)
      { brother_num = *path.items[path.top-1].ptr->son(path.items[path.top-1].pos-1, oper.inner_step);  // left brother
        if (!eif.read(brother_num, brother_node, oper)) 
          { free_cb_frame(brother_node);  return false; }
        unsigned joined_space = brother_node->used_space + local_data;
        if (!brother_node->is_leaf()) joined_space += oper.inner_step;
        if (joined_space <= cb_cluster_size) // can join with the brother
        { if (!brother_node->is_leaf()) // copy the dividing key value from the upper node:
          { memcpy(brother_node->end_pos(), path.items[path.top-1].ptr->key(path.items[path.top-1].pos, oper.inner_step), oper.key_value_size);
            brother_node->used_space += oper.key_value_size;
           // add the pointer:
            *(cbnode_number*)brother_node->end_pos() = curr_node->leftmost_son;
            brother_node->used_space += sizeof(cbnode_number);
          }
         // move the values:
          memcpy(brother_node->end_pos(), curr_node->key(0, 0), local_data);
          brother_node->used_space += local_data;
          joined=true;  last_join_was_left=true;
        }
      }
     // then, try the right brother:
      cbnode_number * brother_num_ptr = path.items[path.top-1].ptr->son(path.items[path.top-1].pos+1, oper.inner_step);
      if (!joined && brother_num_ptr < path.items[path.top-1].ptr->end_pos())  // right brother exists
      { brother_num = *brother_num_ptr;
        if (!eif.read(brother_num, brother_node, oper)) 
          { free_cb_frame(brother_node);  return false; } 
        unsigned joined_space = brother_node->used_space + local_data;
        if (!brother_node->is_leaf()) joined_space += oper.inner_step;
        if (joined_space <= cb_cluster_size) // can join with the brother
        { if (!brother_node->is_leaf()) 
          {// make space in the brother node: 
            memmove(brother_node->key(0, 0)+local_data+oper.inner_step, brother_node->key(0, 0), brother_node->used_space-sizeof(cb_node));
           // copy the values from the current path node:
            memcpy(brother_node->key(0, 0), curr_node->key(0, 0), local_data);
           // copy the dividing key value from the upper node:
            memcpy(brother_node->key(0, 0)+local_data, path.items[path.top-1].ptr->key(path.items[path.top-1].pos+1, oper.inner_step), oper.key_value_size);
           // add the pointer:
            *(cbnode_number*)(brother_node->key(0, 0)+local_data+oper.key_value_size) = brother_node->leftmost_son;
            brother_node->leftmost_son = curr_node->leftmost_son;
            brother_node->used_space += oper.inner_step + local_data;
          }
          else  // joining leaves
          {// make space in the brother node: 
            memmove(brother_node->key(0, 0)+local_data, brother_node->key(0, 0), brother_node->used_space-sizeof(cb_node));
           // copy the values from the current path node:
            memcpy(brother_node->key(0, 0), curr_node->key(0, 0), local_data);
            brother_node->used_space += local_data;
          }
          joined=true;  last_join_was_left=false;
        }
      }
      path.check();
     // write changes and continue:
      if (joined)
      {// write and unlock the brother: 
        eif.write(brother_num, brother_node);
        free_cb_frame(brother_node);
       // deallocate and unlock the path node, make the path shorter:
        eif.free_cbnode(path.items[path.top].num);
        path.pop();
       // NOTE: not writing [curr_node].
      }
      else
      { free_cb_frame(brother_node);
       // write and unlock the path node:
        eif.write(path.items[path.top].num, curr_node);
        //path.pop();  // not necessary, will be done in the path destructor
        break;
      }
    }
    else  // value removed from the root node
    { if (!curr_node->is_leaf() && local_data==0)  // remove the top, but put the its son's contents into it
      { cbnode_number son_num = curr_node->leftmost_son; // son_num must be saved while curr_node is overwritten
       // just read the son node into the root frame: 
        if (!eif.read(son_num, curr_node, oper)) return false;
       // deallocate the son:
        eif.free_cbnode(son_num);
       // write the root with the new contents:
        eif.write(path.items[path.top].num, curr_node);
      }
      else  // just write the chaged root node
        eif.write(path.items[path.top].num, curr_node);
      break;
    }
    path.check();
  } while (true);
  path.check();
  return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if WBVERS>=950
///////////////////// index file access ////////////////////////////////
// File structure: cluster 0 is the header, cluster 1 is the 1st pool, every cluster 1+8*cb_cluster_size is the pool.
// block number = cluster+1
// Bit 1 in the pool: the block is free.
// No caching of the blocks. Immediate saving of changed pool contents in every alloc/free operation.

ext_index_file::ext_index_file(void)
{ handle = INVALID_FHANDLE_VALUE;
  loaded_pool_number = -1;  
  allocated_counter_values=0;  
  err_magic=err_used_space=err_read=err_alloc=err_stack=err_order=0;
  InitializeCriticalSection(&cs_file);
  backup_support=NULL;  // not NULL during the hot backup
  nodes_in_cache=0;
}

ext_index_file::~ext_index_file(void)
{ DeleteCriticalSection(&cs_file); }

bool ext_index_file::open(const char * fname)
{
#ifdef WINS
  handle=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
  // backup needs FILE_SHARE_READ
#else
  handle=::_lopen(fname, OF_READWRITE|OF_SHARE_EXCLUSIVE);
#endif
  loaded_pool_number = -1;  
  nodes_in_cache = 0;
  if (!FILE_HANDLE_VALID(handle)) return false;
  uns64 len64;
#ifdef WINS
  DWORD length=GetFileSize(handle, (DWORD*)&len64+1);
  *(DWORD*)&len64=length;
#else  // LINUX
  len64=GetFileSize(handle, NULL);
#endif
  block_count = (cbnode_number)(len64/cb_cluster_size);  
  return true;
}

bool ext_index_file::create(const char * fname)
// Creates and inits, but does not leave the file open.
{ bool init_ok;
#ifdef WINS
  handle=CreateFile(fname, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL);
#else
  handle=_lcreat(fname, 0666);
#endif
  if (FILE_HANDLE_VALID(handle))  
  { loaded_pool_number = -1;  // when index file is re-created, the loaded pool must not be used any longer
    nodes_in_cache = 0;
    init_ok = init_ext_ft_index();
    close();
  }
  else 
    init_ok=false;
  allocated_counter_values=0;  // must be cleared when the index is re-created!
  return init_ok;
}

void ext_index_file::close(void)
{ if (FILE_HANDLE_VALID(handle))
  {// empty the node cache:
    while (nodes_in_cache)
    { int pool_number = node_cache[0] / (8*cb_cluster_size);
      if (!read_pool(pool_number))
        continue;  // internal error
      save_to_current_pool();  
    }
   // close the file: 
#ifdef WINS
    CloseHandle(handle);
#else
    _lclose(handle);
#endif
    handle = INVALID_FHANDLE_VALUE;
  }  
}

bool ext_index_file::position(cbnode_number number) const
// Called inside cs_file.
{
  number++;  // header block not counted
#ifdef WINS
  unsigned _int64 off64 = (unsigned _int64)number*cb_cluster_size;
  return SetFilePointer(handle, (DWORD)off64, ((long*)&off64)+1, FILE_BEGIN) != (DWORD)-1 || GetLastError()==NO_ERROR;
#elif defined(LINUX)
  return SetFilePointer64(handle, (uns64)number*cb_cluster_size, FILE_BEGIN) != (off_t)-1;
#else
  return SetFilePointer(handle, number*cb_cluster_size, NULL, FILE_BEGIN) != (DWORD)-1;
#endif
}


bool ext_index_file::read0(cbnode_number number, cbnode_ptr ptr) 
{ DWORD rd;
  ProtectedByCriticalSection cs(&cs_file);
  if (backup_support)
  {// try reading from the file of changes made during the backup:
    if (backup_support->read_block(number, ptr))
      return true;  // read OK
   // check for reading from the part added during the backup - must not read because the fil has not been expanded yet
    if (number+1 >= block_count && number+1 < backup_support->new_db_file_size)
      return true; // the contents is undefined (allocated but has not been written)
  }
  return position(number) && ReadFile(handle, ptr, cb_cluster_size, &rd, NULL) && rd==cb_cluster_size;
}

bool ext_index_file::read(cbnode_number number, cbnode_ptr ptr, cbnode_oper & oper) 
{
  if (!read0(number, ptr))
    return false;
#if 1  // on-line verification
  if (number!=COUNTER_NODE_NUM && (number % (8*cb_cluster_size))!=0)
  { if (ptr->magic!=cbnode_inner_magic && ptr->magic!=cbnode_leaf_magic)
      { err_magic++;  return false; }
    int step, space = ptr->used_space - sizeof(cb_node);
    if (ptr->is_leaf())
      step=oper.leaf_step;
    else
      step=oper.inner_step;
    if ((space % step) != 0)
      { err_used_space++;  return false; }
   }
#endif
  return true;
}

bool ext_index_file::write(cbnode_number number, cbnode_ptr ptr)
// Returns false on error
{ DWORD wr;
#if 1  // on-line verification
  if (number!=COUNTER_NODE_NUM && (number % (8*cb_cluster_size))!=0)
    if (ptr->magic!=cbnode_inner_magic && ptr->magic!=cbnode_leaf_magic)
      { err_magic++;  return false; }
#endif
  { ProtectedByCriticalSection cs(&cs_file);
    if (backup_support)
      if (backup_support->is_phase1())
        return backup_support->write_block(number, ptr);
      else
        backup_support->invalidate_block(number);
    if (position(number) && WriteFile(handle, ptr, cb_cluster_size, &wr, NULL) && wr==cb_cluster_size)
      return true;
    contain_block(number);
  }    
  return false;
}

bool ext_index_file::read_pool(int pool_number)
// Updates [loaded_pool_number] on success.
{ 
  if (!read0(pool_number*8*cb_cluster_size, (cbnode_ptr)pool))
    return false;
  loaded_pool_number = pool_number;
  return true;
}

bool ext_index_file::write_pool(void)
{
  return write(loaded_pool_number*8*cb_cluster_size, (cbnode_ptr)pool);
}

cbnode_number ext_index_file::alloc_cbnode(void)
{ bool go_to_next_pool = false;
  if (!nodes_in_cache)
  { cbnode_number max_alloc = 0;
    do // cycle on pools
    {// load the pool, if not loaded (on the start, after emptying the old pool):
      if (loaded_pool_number == -1 || go_to_next_pool)
      { if (!read_pool(loaded_pool_number+1))
        { loaded_pool_number++;  // not done by read_pool because of read error
         // initialize and write a new pool: 
          memset(pool, 0xff, sizeof(pool));  
          pool[0]=loaded_pool_number==0 ? 0xffffffe0 : 0xfffffffe;  // blocks 4,3,2,1 are preallocated in the 1st pool, block 0 in every pool
          if (!write_pool())
            { loaded_pool_number = -1;  return (cbnode_number)-1; }  // file write error
        }
        pool_search_position=0;
      }  
      else
        if (pool_search_position >= cb_cluster_size/4)  // searching the error
	        pool_search_position=0;
     // find free nodes in the pool:
      bool any_allocation = false;
      int start_pos = pool_search_position;
      do // cycle in the pool
      { if (pool[pool_search_position])
        { int subpos = 0;
          while (subpos<32 && nodes_in_cache<CB_NODE_CACHE_SIZE-5)
          { if (pool[pool_search_position] & (1 << subpos)) 
            { cbnode_number alloc = loaded_pool_number*8*cb_cluster_size + 32*pool_search_position + subpos;
              node_cache[nodes_in_cache++] = alloc;
             // remove the free block from the pool:
              pool[pool_search_position] &= ~(1 << subpos);
              if (alloc>max_alloc) max_alloc=alloc;
              any_allocation=true;
            }  
            subpos++;
          }  
          if (nodes_in_cache>=CB_NODE_CACHE_SIZE-5)
            break;
        }
        if (++pool_search_position >= cb_cluster_size/4)
          pool_search_position=0;
      } while (start_pos != pool_search_position);
     // save the pool, if anythig allocated from it: 
      if (any_allocation)
        if (!write_pool())  // file error
          { nodes_in_cache=0;  return (cbnode_number)-1; }
      go_to_next_pool=true;    
     // no free block found in the loaded pool, try the next one:
    } while (!nodes_in_cache);  // not openning the new pool, if anything allocated from the old ones
   // expand the file size or register the increased size, if necessary:   
    if (backup_support)  // if the file is frozen for backup
    { if (backup_support->new_db_file_size <= max_alloc+1)
        backup_support->new_db_file_size = max_alloc+2;
    }
    else
    { ProtectedByCriticalSection cs(&cs_file);  
      contain_block(max_alloc);  // block [resnum] may not be contained in the file, but it may be freed later
    }  
  } // the cache has been filled from pools
  return node_cache[--nodes_in_cache];
}

void ext_index_file::free_cbnode(cbnode_number number)
{
  if (number+1>=block_count)
    { dbg_err("Releasing nonex cb block");  return; }
  if (nodes_in_cache==CB_NODE_CACHE_SIZE)
  { if (loaded_pool_number!=-1)
      save_to_current_pool();
    if (nodes_in_cache==CB_NODE_CACHE_SIZE)  
    { int pool_number = node_cache[0] / (8*cb_cluster_size);
      if (!read_pool(pool_number))
        return;  // internal error
      save_to_current_pool();  
    }
  }
  node_cache[nodes_in_cache++] = number;  
}
    
void ext_index_file::save_to_current_pool(void)
// Saves blocks from the cache which belong to the loaded pool.
{ int i=0;
 // nove nodes from the cache to the loaded pool:
  while (i<nodes_in_cache)
    if (node_cache[i] / (8*cb_cluster_size) == loaded_pool_number)
    {// insert the block into the pool:
      unsigned inpool = node_cache[i] % (8*cb_cluster_size);
      unsigned poolind = inpool/32;
      pool[poolind] |= (1 << (inpool % 32));
     // move the cache contents:
      memmov(node_cache+i, node_cache+i+1, sizeof(cbnode_number)*(nodes_in_cache-i-1));
      nodes_in_cache--;
    // move down the pool search position - it limits the growth of the file:
      if (poolind < pool_search_position) pool_search_position=poolind;
    }
    else i++;  
 // save the pool:
  write_pool();
}

bool ext_index_file::init_ext_ft_index(void)
// Creates the basic structures for the external FT index. The file must be open.
// The root nodes and the counter node have fixed numbers, they cannot be allocated in the normal way.
{ bool res=true;
  cbnode_ptr node = alloc_cb_frame();
 // create the 1st pool (must be before any block is written): 
  free_cbnode(alloc_cbnode());
 // create root nodes:
  for (int i=0;  i<3 && res;  i++)
  { cbnode_number root = i+1;
    node->init_root(true);
    if (!write(root, node)) res=false;
  }
 // counter node:
  memset(node, 0, cb_cluster_size);
  if (!write(COUNTER_NODE_NUM, node)) res=false;
  free_cb_frame(node);
  return res;
}

counter_type ext_index_file::get_counter_value(void)
{
  if (!allocated_counter_values)
  {// allocate new values
    cbnode_ptr node = alloc_cb_frame();
    read0(COUNTER_NODE_NUM, node);  // fictive oper
    next_counter_value = *(counter_type*)node;
    allocated_counter_values = 50;
    *(counter_type*)node = next_counter_value + allocated_counter_values;
    write(COUNTER_NODE_NUM, node);
    free_cb_frame(node);
  }
  allocated_counter_values--;
  return next_counter_value++;
}

void ext_index_file::contain_block(cbnode_number number)
// Called inside cs_file.
// Fulltext is supposed to be locked
{
  if (number+1 >= block_count) 
  { block_count=number+2;
    block_count= 20*((block_count/20)+1);
    uns64 off64 = (uns64)block_count * cb_cluster_size;
#ifdef WINS
    SetFilePointer(handle, (DWORD)off64, ((long*)&off64)+1, FILE_BEGIN);
    SetEndOfFile(handle);
#else
    ftruncate(handle, off64);
#endif
  }
}

int ext_index_file::backup_it(cdp_t cdp, const char * fname, bool nonblocking, const char * schema_name, const char * ft_name, bool reduce)
// Backups the external index file, returns the error number or 0 iff OK. Writes system errors to the log.
// [fname] contains path and the beginning of the target file name.
{ int err=0;
 // make copy:
  char extfname[MAX_PATH], fname1[MAX_PATH];
  get_fulltext_file_name(cdp, extfname, schema_name, ft_name);
  sprintf(fname1, "%s.%s.%s" EXT_FULLTEXT_SUFFIX, fname, schema_name, ft_name);
  
  if (nonblocking)
  { { ProtectedByCriticalSection cs2(&cs_file);
      backup_support = new t_backup_support_ftx(&cs_file, this, schema_name, ft_name);
      if (backup_support)
      { if (!backup_support->prepare(false))
        { propagate_system_error("Error %s when openning auxiliary FT hot-backup file %s.", backup_support->my_auxi_file_name());
          delete backup_support;  backup_support=NULL; 
          return OS_FILE_ERROR;
        }
      }
      else err=OUT_OF_KERNEL_MEMORY;
    }      
    if (backup_support)  // auxiliary file found and opened
    {// backup (all write operations are redirected):
      { t_temp_priority_change tpc(false);
        if (!CopyFileEx(extfname, fname1, NULL, NULL, &bCancelCopying, 0))
        { propagate_system_error("Error %s when hot-backing up the external FT to %s.", fname1);
          err=OS_FILE_ERROR;
        }  
      }
     // synchronize changes:
      backup_support->synchronize(false, (void**)&backup_support);
    }
  }
  else
  { t_writer mrsw_writer(&mrsw_lock, cdp);
    { t_temp_priority_change tpc(true);
      if (!CopyFileEx(extfname, fname1, NULL, NULL, &bCancelCopying, 0))
      { propagate_system_error("Error %s when backing up the external FT to %s.", fname1);
        err=OS_FILE_ERROR;
      }
    }  
  }
 // delete old backup files, if the number is limited:
  if (reduce)
  { char fname[MAX_PATH];  // creating fictive file name
    strcpy(fname, the_sp.BackupDirectory.val());  append(fname, "01020304.05");
    backup_count_limit(fname, fname1+strlen(fname));
  }  
  return err;
}

void ext_index_file::complete_broken_backup(const char * schema_name, const char * ft_name)
{
#if WBVERS>=1000
  backup_support = new t_backup_support_ftx(&cs_file, this, schema_name, ft_name);
  if (backup_support)
  { // look for the auxiliary file:
    if (!backup_support->prepare(true))
      { delete backup_support;  backup_support=NULL; }
    else  // auxiliary file found and opened
    { // write the changes form the auxi file and delete it:
      backup_support->synchronize(true, (void**)&backup_support);
    }
  }    
#endif
}
///////////////////////////////// creating btree from ordered nodes ////////////////////////////////////////
bool cbnode_path::start_ordered_creating(ext_index_file * eif, cbnode_oper * oper)
{ top=0;
  items[0].num = eif->alloc_cbnode(); 
  if (!items[0].num) return false;
  items[0].ptr = alloc_cb_frame();
  if (!items[0].ptr) return false;
  items[0].ptr->init_root(true);
  items[0].pos = 0;
  return true;
}

bool cbnode_path::cb_add_ordered(ext_index_file * eif, cbnode_oper * oper, const char * keyandval)
// path is reversed: items[0] is the leaf page, items[top] is the root */
// Starts with top==0, pointing to an existing empty leaf page, pos==0. 
// [keyandval] is added into the current leaf. If full, the nmber of the full leaf and the key value is passed upwards.
// Page number and key value are stored in the inner page. When the page is full, only the page number is stored into it,
// and the page is replaced by a new one. The number of the removed page and the original key are passed upwareds.
// items[].pos in the leaf is the position to be written. In inner pages it is initially -1.
// Every inner page in the path has at least space for one pointer (it is already calculated into used_space).
{ 
  cbnode_ptr node;
  cbnode_number passed_dadr;
  bool newpage;  // cycling while a new page created in the current cycle (its number is [passed_dadr])
 // add to the leaf:
  node = items[0].ptr;
  newpage = node->used_space+oper->leaf_step > cb_cluster_size;
  if (newpage) // save the old page and create a new leaf page, store the number of the original page to [passed_dadr]
  { if (!eif->write(items[0].num, node)) return false;
    passed_dadr = items[0].num;
    items[0].num = eif->alloc_cbnode();
    if (!items[0].num) return false;
    node->init_root(true);
    items[0].pos=0;
  }
 // appending the key and the clustered value to the current leaf page:
  memcpy(node->key(items[0].pos, oper->leaf_step), keyandval, oper->leaf_step);
  node->used_space+=oper->leaf_step;
  items[0].pos++;
 // adding pointer and key in a cycle:
  int sp = 1;  // pointer passing the stack from the leaf (0) to the root
  while (newpage) // on the sp level 
  {
    if (sp>top)  // add next level: new root (inner node) 
    { top++;
      items[top].num = eif->alloc_cbnode(); 
      if (!items[top].num) return false;
      node = items[top].ptr = alloc_cb_frame();
      node->init_root(false);
      items[top].pos = -1;
    }
    else 
      node = items[sp].ptr;
   // save the pointer from the previous level:
    *node->son(items[sp].pos, oper->inner_step) = passed_dadr;
    newpage = node->used_space+oper->inner_step > cb_cluster_size; 
    if (newpage)  // no space in the inner node: saved the pointer only, create a new page
    { if (!eif->write(items[sp].num, node)) return false;
      passed_dadr = items[sp].num;
      items[sp].num = eif->alloc_cbnode();
      if (!items[sp].num) return false;
      node->init_root(false);
      items[sp].pos=-1;
    }
    else  // save the key value in the inner node and stop
    { items[sp].pos++;
      memcpy(node->key(items[sp].pos, oper->inner_step), keyandval, oper->key_value_size);
      *node->son(items[sp].pos, oper->inner_step) = (cbnode_number)-1;  // temp.
      node->used_space += oper->inner_step;
    }
    sp++;
  } 
  return true;
}

bool cbnode_path::close_ordered_creating(ext_index_file * eif, cbnode_oper * oper, cbnode_number root)
{ cbnode_ptr node;
  cbnode_number passed_dadr;
 // replace the root number:
  eif->free_cbnode(items[top].num);
  items[top].num = root;
 // save the leaf: 
  passed_dadr = items[0].num;
  node = items[0].ptr;
  if (!eif->write(passed_dadr, node)) return false;
  free_cb_frame(node);
  items[0].ptr = NULL;
 // save inner nodes:
  for (int sp=1;  sp<=top;  sp++)
  { node = items[sp].ptr;
    *node->son(items[sp].pos, oper->inner_step) = passed_dadr;
    passed_dadr = items[sp].num;
    if (!eif->write(passed_dadr, node)) return false;
    free_cb_frame(node);
    items[sp].ptr=NULL;
  }
  top=-1;  // path is empty
  return true;
}

#endif
/////////////////////////////////// sort from the scratch /////////////////////////////////////
bool t_scratch_sorter::prepare(cdp_t cdp)
// Allocates the main memory block for sorting 
// Returns TRUE if cannot alocate. 
// keysize must be defined on entry.
// "used_sort_space" is not protected by a critical section, errors are almost harmles.
{ unsigned size_llimit, estimated_space;
 // alocate working space for 2 nodes:
  medval  =(tptr)corealloc(keysize, 44);
  keyspace=(tptr)corealloc(keysize, 44);
  last_key_on_output=(tptr)corealloc(keysize, 44);
  if (!medval || !keyspace || !last_key_on_output)
    { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return true; }
 // calculate the bigblock size:
	estimated_space = 20000000;
  // allocation upper limit: minimum from sort space defined in the profile, rest of the total sort space, estimated necessary space:
  unsigned allocable_space = the_sp.total_sort_space.val() > used_sort_space ? 1024L*(the_sp.total_sort_space.val() - used_sort_space) : 0;
  unsigned profile_space   = 1024L * the_sp.profile_bigblock_size.val();
  bbsize = estimated_space;
  if (bbsize > allocable_space) bbsize = allocable_space;
  if (bbsize > profile_space)   bbsize = profile_space;
  // alocation lower limit: 4*cb_cluster_size nesessary for mergesort (3*cb_cluster_size is the real minimum, but it is horribly inefficiet)
  size_llimit=4*cb_cluster_size;
  if (bbsize < size_llimit) bbsize = size_llimit;
 // allocating the bigblock:
	do
	{ bigblock_handle=aligned_alloc(bbsize, &bigblock);
		if (bigblock_handle) break;
		bbsize=bbsize/4*3;
		if (bbsize < size_llimit)
		  { request_error(cdp, OUT_OF_KERNEL_MEMORY);  return true; }
	} while (true);
 // register the allocated bigblock space:
  used_sort_space += bbsize / 1024;
 // compute space-related koefs:
  bbrecmax=bbsize / keysize;
  bbframes=bbsize / cb_cluster_size - 1; // >=3
  if (bbframes > chains.MAX_EVID / 2 - 3) bbframes = chains.MAX_EVID / 2 - 3;
  if (bbframes > MAX_MERGED) bbframes = MAX_MERGED;
  chains.init(bbframes);
 // prepare path:
  return !output_path.start_ordered_creating(eif, oper);
}

void t_scratch_sorter::quick_sort_bigblock(int start, int stop)
// start < stop supposed
{ int median;
 // find median:
  median=(start+stop)/2;
  if (start+7 < stop) // optimize the median
  { int ma=start+(stop-start+1)/4, mb=start+(stop-start+1)/4*3;
    int ram=oper->cmp_keys(node(ma),     node(median));
    int rmb=oper->cmp_keys(node(median), node(mb));
    if (ram<0)
      if (rmb>0) // ma, mb less, take bigger of them
      { int rab=oper->cmp_keys(node(ma),     node(mb));
        if (rab<0) median=mb; else median=ma;
      }
      else
        ;
    else // ram>0
      if (rmb<0) // ma, mb bigger, take less of them
      { int rab=oper->cmp_keys(node(ma),     node(mb));
        if (rab<0) median=ma; else median=mb;
      }
      else
        ;
  }
 // dividing:
  int i=start, j=stop;
  memcpy(medval, node(median), keysize);
  do // INV: <start..i) values <= median value && (j..stop> values >= median value
  { while (i<=stop  && oper->cmp_keys(node(i), medval) < 0) i++; 
    while (j>=start && oper->cmp_keys(medval, node(j)) < 0) j--; 
    if (i<=j)
    { if (i<j)
      { memcpy(keyspace, node(i),  keysize);
        memcpy(node(i),  node(j),  keysize);
        memcpy(node(j),  keyspace, keysize);
      }
      i++;  j--; 
    }
  } while (i<=j);
 // recursion:
  if (start < j) quick_sort_bigblock(start, j);
  if (i < stop)  quick_sort_bigblock(i, stop);
}

void t_scratch_sorter::bigblock_completed(void)
// Sorts the bigblock contents and outputs it to the chain.
{// sort it:
  quick_sort_bigblock(0, nodes_in_bigblock-1);
 // output it:
  tblocknum ch = bigblock_to_chain();
 // store the chain in the evid: merge chains if there is no space 
  unsigned level = 0; // the level where the chain ch should be stored
  while (chains.chain_cnt(level)==bbframes) // must merge, goes from the top
  { tblocknum next_ch = merge_chains(level, false);
    chains.null_level(level);  chains.add_chain(level, ch);  
    ch=next_ch;  level++;
  }
  chains.add_chain(level, ch);  
}

void t_scratch_sorter::add_key(const void * key_value, const void * clustered_value)
{
  if (nodes_in_bigblock == bbrecmax)  // sort and save the bigblock
    bigblock_completed();
 // append to the bigblock
  char * ptr = node(nodes_in_bigblock);
  memcpy(ptr, key_value, oper->key_value_size);
  memcpy(ptr+oper->key_value_size, clustered_value, oper->clustered_value_size);
  nodes_in_bigblock++;
}

tblocknum t_scratch_sorter::bigblock_to_chain(void)
// nodes_in_bigblock>0. Creates a chain and returns its head number.
{ cbnode_number current_head=0;
  t_chain_link chl;
  char * bl = (char*)alloc_cb_frame();
  while (nodes_in_bigblock)
  {// find number of nodes to be saved in the current block:
    unsigned current_nodes = nodes_in_bigblock % nodes_per_block; 
    if (!current_nodes) current_nodes=nodes_per_block;
    chl.local_nodes=current_nodes;  chl.next_block=current_head;
   // save into a chain:
    current_head = eif->alloc_cbnode();  cbnodes_in_chains++;
    memcpy(bl, node(nodes_in_bigblock-current_nodes), current_nodes*keysize);
    memcpy(bl+cb_cluster_size-sizeof(t_chain_link), &chl, sizeof(t_chain_link));
    eif->write(current_head, (cbnode_ptr)bl);
    nodes_in_bigblock-=current_nodes;
  }
  free_cb_frame((cbnode_ptr)bl);
  return current_head;
}

bool t_scratch_sorter::optimize_merge(unsigned level)
// If total count of chains is <= bbframes then all chains can be merged in a single step.
// The proc puts all of them on the level "level" and returns TRUE (the final merging will follow)
// Otherwise returns FALSE.
{ unsigned total_chains = 0, level_chains, evid_level;
  for (evid_level=level;  TRUE;  evid_level++)
  { level_chains=chains.chain_cnt(evid_level);
    if (!level_chains) break;
    total_chains+=level_chains;
  }
  if (total_chains <= bbframes) // can merge all in a single step
  { for (evid_level=level+1;  TRUE;  evid_level++)
      if (!chains.chain_cnt(evid_level)) break;
      else // copy chains from upper evid_level to level:
        for (unsigned ch=0;  ch<chains.chain_cnt(evid_level);  ch++)
          chains.add_chain(level, chains.get_chain(evid_level, ch));
    return TRUE;
  }
  return FALSE;
}

void t_scratch_sorter::write_result_chain(bool final_merge, tptr result_block, unsigned result_cnt, 
                 cbnode_number & current_result_blocknum, bool is_last_block)
{ if (final_merge)
    write_result(result_block, result_cnt);
  else
  {// first, allocate the next block, because the current block will contain the link to it:
    tblocknum next_result_blocknum;
    if (is_last_block) 
      next_result_blocknum = 0;
    else
    { next_result_blocknum = eif->alloc_cbnode(); // allocate the next result block (its numbers will be stored in the current block)
      cbnodes_in_chains++;
    }
   // complete the current block and write it:
    t_chain_link * chl = (t_chain_link *)(result_block+cb_cluster_size-sizeof(t_chain_link));
    chl->next_block=next_result_blocknum;  chl->local_nodes=result_cnt;
    eif->write(current_result_blocknum, (cbnode_ptr)result_block);
   // update [current_result_blocknum]:
    current_result_blocknum=next_result_blocknum;
  }
}

tblocknum t_scratch_sorter::merge_chains(unsigned level, bool final_merge)
// merges all the chains on level "level".
{ struct t_toplist_entry
  { unsigned blocknum;      // index to merged_blocks[]
    unsigned pos_in_block;
  } toplist[MAX_MERGED]; // list of references to top nodes of the merged strings, sorted from MAX to MIN
  unsigned i;
 // initialise toplist:
  unsigned top_list_size=0; // number of the non-exhausted merged chains (decreases during the process)
  for (i=0;  i<chains.chain_cnt(level);  i++)
  { merged_blocks[i]=bigblock+i*cb_cluster_size;
    cbnode_number chain_block_num = chains.get_chain(level, i);
    eif->read0(chain_block_num, (cbnode_ptr)merged_blocks[i]);
    eif->free_cbnode(chain_block_num);  cbnodes_in_chains--;
    unsigned j;
    for (j=0;  j<top_list_size;  j++)
      if (oper->cmp_keys(merged_blocks[toplist[j].blocknum], merged_blocks[i]) < 0) break;
    // insert the i-th block into the [j] position in the toplist
    memmov(&toplist[j+1], &toplist[j], (top_list_size-j)*sizeof(t_toplist_entry));
    toplist[j].blocknum=i;  toplist[j].pos_in_block=0;  
    top_list_size++;
  }
 // init the result chain:
  cbnode_number result_head, current_result_blocknum;  // the head and the current block of the 
  tptr result_block, result_pos;  // result block and the current position pointer
  if (final_merge)
    current_result_blocknum = 0; // not used
  else
  { current_result_blocknum = eif->alloc_cbnode();
    cbnodes_in_chains++;
  }
  result_head = current_result_blocknum;
  unsigned result_cnt=0;                           // number of nodes in the current block of the result chain
  result_block=result_pos=bigblock+bbframes*cb_cluster_size; // INV: result_pos==result_block+keysize*result_cnt
 // merge it:
  t_chain_link * chl;
  while (top_list_size) // while any of the input chains is non-empty
  {// make space in the result block:
    if (result_cnt==nodes_per_block)
    { write_result_chain(final_merge, result_block, result_cnt, current_result_blocknum, false);
      result_pos=result_block;  result_cnt=0;
    }
   // copy a node to the result
    t_toplist_entry * top = &toplist[top_list_size-1];
    unsigned source_block=top->blocknum;
    memcpy(result_pos, merged_blocks[source_block]+keysize*top->pos_in_block, keysize);
    result_pos+=keysize;  result_cnt++;
   // get new source node from the same chain, if exists:
    chl = (t_chain_link *)(merged_blocks[source_block]+cb_cluster_size-sizeof(t_chain_link));
    if (++top->pos_in_block >= chl->local_nodes)
    { cbnode_number chain_block_num = chl->next_block;
      if (chain_block_num)
      { eif->read0(chain_block_num, (cbnode_ptr)merged_blocks[source_block]);
        eif->free_cbnode(chain_block_num);  cbnodes_in_chains--;
        top->pos_in_block=0;
      }
      else
      { top_list_size--;
        continue;  // no sorting in the toplist
      }
    }
   // move the new source node to the correct place:
    for (i=0;  i<top_list_size-1;  i++)
    { tptr key = merged_blocks[top->blocknum]+top->pos_in_block*keysize;
      if (oper->cmp_keys(merged_blocks[toplist[i].blocknum]+toplist[i].pos_in_block*keysize, key) < 0) 
      { t_toplist_entry top_copy=*top;
        memmov(&toplist[i+1], &toplist[i], (top_list_size-i-1)*sizeof(t_toplist_entry));
        toplist[i]=top_copy;
        break;
      }
    }
  }
 // write the last result block:
  write_result_chain(final_merge, result_block, result_cnt, current_result_blocknum, true);
  return result_head;
}

void t_scratch_sorter::write_result(tptr result_block, unsigned count)
{ 
  while (count--)
  { if (!distinct || oper->cmp_keys(last_key_on_output, result_block) || 
        memcmp(last_key_on_output+oper->key_value_size, result_block+oper->key_value_size, oper->clustered_value_size))
    { output_path.cb_add_ordered(eif, oper, result_block);
      if (distinct) memcpy(last_key_on_output, result_block, oper->key_value_size);
    }
    result_block+=keysize;
  }
}

void t_scratch_sorter::complete_sort(cbnode_number root)
{ int level;
 // sorts and outputs the last (probably incomplete) bigblock:
  if (nodes_in_bigblock)
    bigblock_completed();
 // merging every level except the last, storing result on upper level:
  for (level=0;  !optimize_merge(level);  level++)
  // INV: all levels below "level" are empty, some level above "level" is non-empty
  { tblocknum next_ch = merge_chains(level, false);  chains.null_level(level);  
   // find a level above "level" which is not full for storing the result chain:
    unsigned result_level=level+1;
    while (chains.chain_cnt(result_level)==bbframes) result_level++;
    chains.add_chain(result_level, next_ch);  
  }
 // final merging and output:
  merge_chains(level, true);
 // save the clustered btree:
  output_path.close_ordered_creating(eif, oper, root);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef WINS

t_mrsw_lock::t_mrsw_lock(void)
{ readers_inside=0;  writer_inside=false;
  InitializeCriticalSection(&acs);
  hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // manual, non-signalled
}

t_mrsw_lock::~t_mrsw_lock(void)
{
  DeleteCriticalSection(&acs);
  CloseHandle(hEvent);
}

void t_mrsw_lock::reader_entrance(cdp_t cdp, t_wait wait_type)
{ bool can_enter;
  prof_time start;
  if (PROFILING_ON(cdp)) start = get_prof_time();
  do
  {// check the conditions:
    EnterCriticalSection(&acs);
    if (writer_inside)
      { can_enter=false;   }
    else
      { can_enter=true;  readers_inside++; }
    LeaveCriticalSection(&acs);
    if (!can_enter)  // wait on the semaphore
    { cdp->wait_type=wait_type;
      cdp->ft_cycles++;
      WaitForSingleObject(hEvent, 2000);  // the other thread can pulse the event BEFORE I start waiting for it
    }  
    else break;
  } while (true);
  cdp->wait_type=WAIT_NO;
  if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_CS, wait_type-100, 0, NULL);
}

void t_mrsw_lock::reader_exit(void)
{
  EnterCriticalSection(&acs);
  readers_inside--;
  PulseEvent(hEvent);
  LeaveCriticalSection(&acs);
}

void t_mrsw_lock::writer_entrance(cdp_t cdp, t_wait wait_type)
{// wait on the semaphore
  bool can_enter;
  prof_time start;
  if (PROFILING_ON(cdp)) start = get_prof_time();
  do
  {// check the conditions:
    EnterCriticalSection(&acs);
    if (writer_inside || readers_inside)
        can_enter=false;
    else
      { can_enter=true;  writer_inside=true; }
    LeaveCriticalSection(&acs);
    if (!can_enter)  // wait on the semaphore
    { cdp->wait_type=wait_type;
      cdp->ft_cycles++;
      WaitForSingleObject(hEvent, 2000);  // the other thread can pulse the event BEFORE I start waiting for it
      GetLastError();
    }
    else break;
  } while (true);
  cdp->wait_type=WAIT_NO;
  if (PROFILING_ON(cdp)) add_hit_and_time(get_prof_time()-start, PROFCLASS_CS, wait_type-100, 0, NULL);
}

void t_mrsw_lock::writer_exit(void)
{
  EnterCriticalSection(&acs);
  writer_inside=false;
  PulseEvent(hEvent);
  GetLastError();
  LeaveCriticalSection(&acs);
}

#endif
