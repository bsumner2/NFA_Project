#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <alloca.h>

#define ERR_PREFIX "\x1b[31m[Error]:\x1b[0m "





#define perrf(fmt, ...)  fprintf(stderr, ERR_PREFIX fmt, __VA_ARGS__)
#define perr(s) fputs( ERR_PREFIX s, stderr)

#define STATE_CT_HEADER_TAG "Number of states: "
#define ALPHABET_SIZE_HEADER_TAG "Alphabet size: "
#define FINAL_STATES_HEADER_TAG "Accepting states: "

typedef struct bst_node {
  int state;
  int height;
  struct bst_node *l, *r;
} BST_Node_t;

typedef struct bst {
  BST_Node_t *root;
  int size;
} BST_t;



void BST_Init(BST_t *stat_inst) {
  stat_inst->size = 0;
  stat_inst->root = NULL;
}

static BST_Node_t *BST_Node_Alloc(int state) {
  BST_Node_t *ret = calloc(1, sizeof(BST_Node_t));
  ret->state = state;
  return ret;
}

static inline int Height(BST_Node_t *root) {
  return root ? root->height : -1;
}

static inline int BalanceFactor(BST_Node_t *root) {
  return !root ? 0 : Height(root->l) - Height(root->r);
}

static void RecalcHeight(BST_Node_t *root) {
  int lh, rh;
  if (!root)
    return;
  root->height = 1+ 
    ((lh = Height(root->l)) > (rh = Height(root->r))
      ? lh : rh);
}

BST_Node_t *BST_Node_LL_Rotate(BST_Node_t *root) {
  // So:
  //           x
  //         y   z
  //        u v
  //       A   
  //        Becomes:
  //           y
  //         u   x
  //        A   v z
  // Where x is old root, y is new root, and A is the newly-added node that disbalanced the tree.
  //
  BST_Node_t *newroot = root->l;
  root->l = newroot->r;
  RecalcHeight(root);
  newroot->r = root;
  RecalcHeight(newroot);
  return newroot;
}

BST_Node_t *BST_Node_LR_Rotate(BST_Node_t *root) {
  // So:
  //            x
  //         y     z
  //       u   v
  //          1 2 
  //        Becomes:
  //           v
  //        y     x
  //       u 1   2 z
  // Where x is OG root, v is new root, and either 1 or 2 is the newly-added node, but never both.
  BST_Node_t *newroot = root->l->r;  // Save v before it becomes detatched from old root
  root->l->r = newroot->l;  // replace v's place with 1, thereby detatching it from old root subtree
  RecalcHeight(root->l);  // since y's leaves have changed, recalculate y's height
  newroot->l = root->l;  // since left node of v is now the right node of y, we can finish the switch and make y the new left node of v
  root->l = newroot->r;  // finish swapping of y by replacing y's old place with 2
  RecalcHeight(root);  // since x's leaves have changed recalc x's height
  newroot->r = root;
  RecalcHeight(newroot);  // Now that we're done messing with newroot's leaves, finally recalc its height before returning.
  return newroot;
}

BST_Node_t *BST_Node_RR_Rotate(BST_Node_t *root) {
  //     x
  //   y   z
  //      u v
  //         A
  // becomes:
  //       z
  //     x   v
  //    y u   A
  BST_Node_t *newroot = root->r;
  root->r = newroot->l;
  RecalcHeight(root);
  newroot->l = root;
  RecalcHeight(newroot);
  return newroot;
}

BST_Node_t *BST_Node_RL_Rotate(BST_Node_t *root) {
  BST_Node_t *newroot = root->r->l;
  root->r->l = newroot->r;
  RecalcHeight(root->r);
  newroot->r = root->r;
  root->r = newroot->l;
  RecalcHeight(root);
  newroot->l = root;
  RecalcHeight(newroot);
  return newroot;
}


static void BST_Node_InOrder_Traversal_Fill(int *dst, BST_Node_t *node, int *next_empty) {
  if (!node)
    return;
  BST_Node_InOrder_Traversal_Fill(dst, node->l, next_empty);
  dst[(*next_empty)++] = node->state;
  BST_Node_InOrder_Traversal_Fill(dst, node->r, next_empty);
}

int *BST_To_Array(BST_t *tree, int *return_len) {
  if ((*return_len = tree->size) == 0) {
    return NULL;
  }
  int *ret = malloc(sizeof(int)*(*return_len));
  int curr = 0;
  BST_Node_InOrder_Traversal_Fill(ret, tree->root, &curr);
  return ret;
}









static BST_Node_t *BST_Node_Insert(BST_Node_t *root, int state, _Bool *rebal_flag) {
  if (!root) {
    root = BST_Node_Alloc(state);
    *rebal_flag = 1;
    return root;
  }
  int diff = state - root->state;
  if (!diff) {
    return root;
  } else if (diff > 0) {
    root->r = BST_Node_Insert(root->r, state, rebal_flag);
  } else {
    root->l = BST_Node_Insert(root->l, state, rebal_flag);
  }

  if (!(*rebal_flag))
    return root;
  diff = BalanceFactor(root);
  if (diff > 1) {
    if (state < root->l->state)
      root = BST_Node_LL_Rotate(root);
    else
      root = BST_Node_LR_Rotate(root);
  } else if (diff < -1) {
    if (state > root->r->state)
      root = BST_Node_RR_Rotate(root);
    else
      root = BST_Node_RL_Rotate(root);
  } else {
    RecalcHeight(root);
  }

  return root;

}



static BST_Node_t *BST_Node_PostDelete_Rebalance(BST_Node_t *root) {
  int bal = BalanceFactor(root);
  if (bal > 1) {
    if (BalanceFactor(root->l)>=0) {
      return BST_Node_LL_Rotate(root);
    } else {
      return BST_Node_LR_Rotate(root);
    }
  } else if (bal < -1) {
    if (BalanceFactor(root->r) <= 0) {
      return BST_Node_RR_Rotate(root);
    } else {
      return BST_Node_RL_Rotate(root);
    }
  } else {
    RecalcHeight(root);
    return root;
  }

}

static BST_Node_t *HibbardDelete(BST_Node_t *root) {
  BST_Node_t *stack[root->r->height + 2];
  BST_Node_t *tmp;
  int top = -1;

  {
   BST_Node_t *smallest_rsub = root->r;
    while (smallest_rsub->l) {
      stack[++top] = smallest_rsub;
      smallest_rsub = smallest_rsub->l;
    }
    root->state = smallest_rsub->state;
    tmp = smallest_rsub->r;
    free(smallest_rsub);
  }
  while (top > -1) {
    stack[top]->l = tmp;
    tmp = BST_Node_PostDelete_Rebalance(stack[top--]);
  }

  root->r = tmp;
  return BST_Node_PostDelete_Rebalance(root);
  

}


static BST_Node_t *BST_Node_Excise(BST_Node_t *root, int state, _Bool *rebal_flag) {
  int diff;
  BST_Node_t *node;
  if (!root) {
    *rebal_flag = 0;
    return root;
  }
  diff = state - root->state;


  if (diff < 0) {
    root->l = BST_Node_Excise(root->l, state, rebal_flag);
  } else if (diff > 0) {
    root->r = BST_Node_Excise(root->r, state, rebal_flag);
  } else {
    *rebal_flag = 1;
    if (!(root->l)) {
      node = root->r;
      free(root);
      return node;
    } else if (!(root->r)) {
      node = root->l;
      free(root);
      return node;
    } else {
      return HibbardDelete(root);
    }
  }

  if (!(*rebal_flag))
    return root;

  return BST_Node_PostDelete_Rebalance(root);

}

void BST_Add(BST_t *tree, int state) {
  _Bool insertion_flag = 0;
  tree->root = BST_Node_Insert(tree->root, state, &insertion_flag);
  if (insertion_flag)
    ++tree->size;
}




void BST_Remove(BST_t *tree, int state) {
  _Bool rm_flag = 0;
  tree->root = BST_Node_Excise(tree->root, state, &rm_flag);
  if (rm_flag)
    --tree->size;
}

_Bool BST_Contains(BST_t *tree, int state) {
  BST_Node_t *root = tree->root;
  int diff;
  while (root) {
    diff = state - root->state;
    if (!diff)
      return 1;
    else if (diff < 0) {
      root = root->l;
      continue;
    } else {
      root = root->r;
      continue;
    }
  }

  return 0;

}

static void BST_Node_Free_Subtree(BST_Node_t *root) {
  if (!root)
    return;

  BST_Node_Free_Subtree(root->l);
  BST_Node_Free_Subtree(root->r);
  free(root);
}

void BST_Close(BST_t *tree) {
  if (!(tree->size)) {
    tree->root = NULL;
    return;
  }
  BST_Node_Free_Subtree(tree->root);
  tree->root = NULL;
  tree->size = 0;
}



typedef struct changeable_enfa {
  BST_t **delta;
  int state_ct;
  int alpha_len;
  BST_t final_states;
} ENFA_t;

/**
 * @param alpha_len include epsilon 
 * */
void Delta_Dealloc(BST_t **delta, int state_ct, int alpha_len) {
  BST_t *curr_state;
  for (int i = 0; i < state_ct; ++i) {
    curr_state = delta[i];
    for (int j = 0; j < alpha_len; ++j) {
      BST_Close(&curr_state[j]);
    }
    free(curr_state);
  }
  free(delta);
}





/* Since */
ENFA_t *Parse_NFA_File(FILE *fp) {
  ENFA_t tmp;
  int state_ct = 0, alpha_len = 0, final_ct = 0;
  fseek(fp, 0, SEEK_END);
  ssize_t flen = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *buf = calloc(flen, sizeof(char));
  
  fread(buf, 1, 18, fp);
  if (strncmp(STATE_CT_HEADER_TAG, buf, 18)) {
    perrf("Misformatted input file.\nExpected: \x1b[1m"
        STATE_CT_HEADER_TAG "\x1b[0m. Received: \x1b[1m%s\x1b[0m\n", buf);
    free(buf);
    return NULL;
  }
  {
    uint64_t *buf64 = (uint64_t*)buf;
    int i = 3;
    while (i--)
      *buf64++ = 0;
  }

  int len = 0;

  {
    char c;
    while ((c=fgetc(fp))!='\n') {
      if (c == EOF) {
        perr("Unexpected EOF.\n");
        free(buf);
        return NULL;
      }
      if (!isdigit(c)) {
        perr("Failed to parse number of states.\n");
        free(buf);
        return NULL;
      }
      buf[len++] = c;
    }

  }
  state_ct = atoi(buf);

  if (!state_ct) {
    perr("Number of states is zero. Nothing to do!\n");
    free(buf);
    return NULL;
  }
  
  while (len--)
    buf[len] = 0;
  
  fread(buf, 1, 15, fp);
  if (strncmp(buf, ALPHABET_SIZE_HEADER_TAG, 15)) {
    perrf("Alphabet header tag malformatted\nExpected: \x1b[1m" 
        ALPHABET_SIZE_HEADER_TAG "\x1b[0m Received: \x1b[1m%s\x1b[0m\n", buf);
    free(buf);

    return NULL;
  }
  
  {
    uint64_t *buf64 = (uint64_t*)buf;
    buf64[0] = buf64[1] = 0;
  }
  {
    char c;
    len = 0;
    while ((c=fgetc(fp))!='\n') {
      if (c==EOF) {
        perr("Unexpected EOF.\n");
        free(buf);
        return NULL;
      }
      if (!isdigit(c)) {
        perr("Failed to parse alphabet size.\n");
        free(buf);
        return NULL;
      }

      buf[len++] = c;
    }


  }

  alpha_len = atoi(buf);
  
  if (!alpha_len) {
    perr("Alphabet len is 0. Nothing to do!\n");
    free(buf);
    return NULL;
  }

  while (len--)
    buf[len] = 0;

  fread(buf, 1, 18, fp);
  if (strncmp(buf, FINAL_STATES_HEADER_TAG, 18)) {
    perrf("Misformatted final states header tag.\nExpected: \x1b[1m" 
        FINAL_STATES_HEADER_TAG "\x1b[0m. Received: \x1b[1m%s\x1b[0m\n", buf);
    free(buf);
    return NULL;
  }
  {
    char c;
    len = 0;
    final_ct = 1;
    c=fgetc(fp);
    if (c=='\n') {
      final_ct = 0;
    } else if (!isdigit(c)) {
      perr("Failed to parse out final state set.\n");
      free(buf);
      return NULL;
    } else {
      fseek(fp, -1, SEEK_CUR);
      while ((c=fgetc(fp))!='\n') {
        if (c==EOF) {
          perr("Unexpected EOF\n");
          free(buf);
          return NULL;
        }
        if (c==' ' || c == '\t') {
          buf[len++] = 0;
          final_ct++;
          continue;
        }
        if (!isdigit(c)) {
          perr("Failed to parse out final state set.\n");
          free(buf);
          return NULL;
        }
        buf[len++] = c;
      }
    }
  }    
  BST_t finals = {0};
  if (final_ct) {
    char *cur = buf;
    
    for (int i = 0; i < final_ct; ++i) {
      BST_Add(&finals, atoi(cur));
      while (*cur++)
        continue;
    }
  }
  memset(buf, 0, len);
  {
    ssize_t curr = ftell(fp);
    fseek(fp, 0, SEEK_END);
    ssize_t end = ftell(fp);
    fseek(fp, curr, SEEK_SET);
    len = end-curr;
    fread(buf, 1, len, fp);
  }
  



  int transitions_len;
  

  char *transitions[(transitions_len = state_ct*(alpha_len+1))];
  transitions[0] = strtok(buf, " \t\r\n");
  for (int i=1; i < transitions_len; ++i) {
    if (!(transitions[i] = strtok(NULL, " \t\r\n"))) {
      perr("Unexpected EOF.");// Transition substrings parsed so far");
      //for (int j = 0; j < i; ++j) {
      //  fprintf(stderr, "transitions[%d] = delta(%d, %c): %s\n", j, j/(alpha_len+1), (j%(alpha_len+1) ? 'a'+(j%(alpha_len+1))-1:'_'), transitions[j]);
      //}
      free(buf);
      BST_Close(&finals);
      return NULL;
    }
  }

  BST_t **delta = calloc(state_ct, sizeof(void*));
  BST_t *curr_state, *curr_set;
  char *tset;
  char *cur;
  int state;
  for (int i = 0; i < state_ct; ++i) {
    delta[i] = curr_state = calloc(alpha_len + 1, sizeof(BST_t));

    for (int j = 0; j < alpha_len+1; ++j) {
      curr_set = &curr_state[j];
      tset = transitions[i*(alpha_len+1)+j];
      if (!strncmp(tset, "{}", 2)) {
        continue;
      }
      cur = strtok(tset, "{},");
      state = atoi(cur);
      
      if (!state && strncmp(cur, "0\0", 2)) {
        perrf("Parse failure at delta(%d, %c). atoi(\"%s\") gave 0.\n",
            i, j?j-1+'a':'_', cur);
        free(buf);
        BST_Close(&finals);
        for (int k = 0; k <= j; ++k) {
          BST_Close(&curr_state[k]);
        }
        free(curr_state);
        Delta_Dealloc(delta, i, alpha_len+1);
        return NULL;
      }
      BST_Add(curr_set, state);
      while ((cur = strtok(NULL, "{}, \t"))) {
        state = atoi(cur);
        if (!state && strncmp(cur, "0\0", 2)) {
          perrf("Parse failure at delta(%d, %c). atoi(\"%s\") gave 0.\n", 
              i, j?j-1+'a':'_', cur);
          free(buf);
          BST_Close(&finals);
          for (int k = 0; k <= j; ++k) {
            BST_Close(&curr_state[k]);
          }
          free(curr_state);
          Delta_Dealloc(delta, i, alpha_len+1);
          return NULL;
        }
        BST_Add(curr_set, state);
      }
    }
  }

  free(buf);
  
  tmp.state_ct = state_ct;
  tmp.alpha_len = alpha_len;
  tmp.delta = delta;
  tmp.final_states = finals;
  ENFA_t *ret = malloc(sizeof(*ret));
  *ret = tmp;
  return ret;
}


void Dealloc_ENFA(ENFA_t *nfa) {
  Delta_Dealloc(nfa->delta, nfa->state_ct, nfa->alpha_len+1);
  BST_Close(&(nfa->final_states));
  free(nfa);
}

_Bool BST_Node_Crosscheck_Trees(BST_Node_t *root, BST_t *crosscheck) {
  if (!root)
    return 0;
  if (BST_Node_Crosscheck_Trees(root->l, crosscheck))
    return 1;
  if (BST_Contains(crosscheck, root->state))
    return 1;
  else
    return BST_Node_Crosscheck_Trees(root->r, crosscheck);

}

_Bool BST_CheckOverlap(BST_t *a, BST_t *b) {
  if (!(b->size))
    return 0;
  if (b->size > a->size)
    return BST_Node_Crosscheck_Trees(a->root, b);
  else
    return BST_Node_Crosscheck_Trees(b->root, a);
}


void BST_Combine(BST_t *dst, BST_t *src) {
  BST_Node_t *stack[src->size], *curr;
  int top = -1;
  if (src->root)
    stack[++top] = src->root;
  while (top > -1) {
    curr = stack[top--];
    if (curr->l)
      stack[++top] = curr->l;
    if (curr->r)
      stack[++top] = curr->r;
    BST_Add(dst, curr->state);
  }
}



int *GetInverseEpsilonClosure(BST_t **delta, int state, int state_ct, int *return_len) {
  int init_len;
  BST_t static_bst_instance = {0}, 
        *inverse_e_closure = &static_bst_instance;

  for (int i = 0; i < state_ct; ++i) {
    if (state == i)
      continue;
    if (BST_Contains(&delta[i][0], state))
      BST_Add(inverse_e_closure, i);
  }
  if (inverse_e_closure->size == 0) {
    *return_len= 0;
    return NULL;
  }
  int *closures = NULL;
  do {
    free(closures);
    closures = BST_To_Array(inverse_e_closure, &init_len);
    int curr = 0;
    for (int i = 0; i < state_ct; ++i) {
      if (state == i)
        continue;
      if (closures[curr] == i) {
        ++curr;
        continue;
      }
      if (BST_CheckOverlap(&delta[i][0], inverse_e_closure)) {
        BST_Add(inverse_e_closure, i);
      }
    }
  } while (inverse_e_closure->size != init_len);
  *return_len = init_len;
  BST_Close(inverse_e_closure);

  return closures;
}
//
// Given a state q,
// For each state r, in inverse_e_closure(q)
//     For each symbol a, in SIGMA
//         delta(r, a) = delta(r, a) U delta(q, a)
void ENFA_InverseEClosure_Add_Bypasses(int q, int *inv_ecls_q, int inv_ecls_len, BST_t **delta, int alpha_len) {
  int r;
  ++alpha_len;  // to account for epsilon
  
  for (int i = 0; i < inv_ecls_len; ++i) {
    r = inv_ecls_q[i];
    for (int j = 1; j < alpha_len; ++j) {  // start at j=1 skips delta(<state>, epsilon)
      BST_Combine(&delta[r][j], &delta[q][j]);
    }
  }
}





void ENFA_ConvertToNFA(ENFA_t *enfa) {
  BST_t **delta = enfa->delta;
  BST_t *finals = &(enfa->final_states);
  int state_ct = enfa->state_ct,
      alpha_len = enfa->alpha_len;
  {
    int init_size;
    do {
      init_size = finals->size;
      for (int i = 0; i < enfa->state_ct; ++i) {
        // So, delta[i][0]  is delta(STATE(i), EPSILON). Ergo, if the Binary State Tree, delta[i][0],
        // contains at least one node with state contained in finals tree, then we make STATE(i) part of
        // final states set, too.
        if (BST_CheckOverlap(&delta[i][0], finals)) { 
          BST_Add(finals, i);
        }
      }
    } while (init_size != finals->size);
  }

//  int *inverse_e_closures[state_ct];
//  int inv_e_close_lens[state_ct];

  for (int i = 0; i < state_ct; ++i) {
    int *inv_eclosure, len;
    inv_eclosure = GetInverseEpsilonClosure(delta, i, state_ct, &len);
    ENFA_InverseEClosure_Add_Bypasses(i, inv_eclosure, len, delta, alpha_len);
    free(inv_eclosure);
  }

  for (int i = 0; i < state_ct; ++i) {
    BST_Close(delta[i]);
  }



}

void Print_NFA_Detes(ENFA_t *nfa) {
  printf("Number of states: %d\n"
      "Alphabet size: %d\n"
      "Accepting states:", nfa->state_ct, nfa->alpha_len);
  {
    int *finals, finals_len;
    finals = BST_To_Array(&(nfa->final_states), &finals_len);
    for (int i = 0; i < finals_len; ++i) {
      printf(" %d", finals[i]);
    }
    free(finals);
  }
  putchar('\n');
  {
    BST_t *curr_state;
    int *curr_set, state_ct = nfa->state_ct, alpha_size = nfa->alpha_len, curr_set_len;
    for (int i = 0; i < state_ct; ++i) {
      curr_state = nfa->delta[i];
      for (int j = 0; j < alpha_size; ++j) {
        curr_set = BST_To_Array(&curr_state[j], &curr_set_len);
        if (!curr_set_len) {
          fputs("{}\t", stdout);
          continue;
        } 
        printf("{%d", curr_set[0]);
        for (int k = 1; k < curr_set_len; ++k) {
          printf(",%d", curr_set[k]);
        }
        free(curr_set);
        fputs("}\t", stdout);
      }
      curr_set = BST_To_Array(&curr_state[alpha_size], &curr_set_len);
      if (!curr_set_len) {
        puts("{}");
        continue;
      }
      printf("{%d", curr_set[0]);
      for (int j = 1; j < curr_set_len; ++j) {
        printf(",%d", curr_set[j]);
      }
      free(curr_set);
      putchar('}');
      putchar('\n');
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    perrf("No Epsilon-NFA file given.\nUsage: %s <input epsilon nfa file>\n", 
        argv[0]);
    return -1;
  }
  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    perror(ERR_PREFIX"failed to open the file: ");
    return -1;
  }

  ENFA_t *enfa = Parse_NFA_File(fp);
  

  fclose(fp);
  if (enfa==NULL)
    return -1;
  
  ENFA_ConvertToNFA(enfa);


  Print_NFA_Detes(enfa);
  
  Dealloc_ENFA(enfa);

  return 0;
}
