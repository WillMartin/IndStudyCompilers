#include "optimization.h"

// Result must be freed
char *hash_arg(Arg *arg)
{
    char *repr;
    if (arg->type == CONST)
    {
        repr = malloc(1+sizeof(int));
        sprintf(repr, "%d", arg->const_val->int_val); 
    }
    else if (arg->type == IDENT)
    {
        repr = malloc(strlen(arg->ident_val->symbol)+1);
        sprintf(repr, "%s", arg->ident_val->symbol);
    }
    else { assert(false); }
    return repr;
}

char *hash_instr(Instruction *instr)
{
    char *repr;// = malloc(sizeof(4));
    // Hashing Ident is trivial, use its symbol
    if (instr->op_code == ASSIGN)
    {
        char *arg1_repr = hash_arg(instr->arg1);
        repr = malloc(strlen(arg1_repr)+2);
        sprintf(repr, "%d%s", instr->op_code, arg1_repr);
        free(arg1_repr);
    }
    else if (!(instr->op_code==GOTO || is_relative_op(instr->op_code)))
    {
        char *arg1_repr = hash_arg(instr->arg1);
        char *arg2_repr = hash_arg(instr->arg2);
        repr = malloc(strlen(arg1_repr)+strlen(arg2_repr)+2);
        sprintf(repr, "%d%s%s", instr->op_code, arg1_repr, arg2_repr);
        free(arg1_repr);
        free(arg2_repr);
    }
    else
    { 
        assert(false);
        // Alternative is a conditional which will be the last element and it 
        // shouldn't be being hashed!
    }
    return repr;
}


// Only for Identifiers!
DagNode *get_most_recent_or_new(GHashTable *most_recents, Arg *arg)
{
    // Check where the most recent copy of it is
    DagNode *recent_node = g_hash_table_lookup(most_recents, arg->ident_val->symbol);

    if (recent_node == NULL)
    {
        // This is an initial identity (it came from the last block)
        recent_node = malloc(sizeof(DagNode));
        recent_node->op_code = NONE;
        recent_node->visited = false;
        recent_node->is_id = true;
        recent_node->ids = g_list_prepend(NULL, arg->ident_val);
        recent_node->init_arg = arg; 
        recent_node->left = NULL;
        recent_node->right = NULL;
        g_hash_table_insert(most_recents, arg->ident_val->symbol, recent_node);
    }
    return recent_node;
}

// Returns a DagNode for the given input arg, either new or from the most_recent table
DagNode *get_arg_node(GHashTable *most_recents, GHashTable* dag_table, Arg* arg)
{
    // Couldn't find a previous node. We need to create a new one.
    DagNode *ret_node;
    if (arg->type == CONST)
    {
        ret_node = malloc(sizeof(DagNode)); 
        //ret_node->op_code = ASSIGN;
        ret_node->visited = false;
        ret_node->is_id = false;
        ret_node->init_arg = arg;
        ret_node-> ids = NULL;
        ret_node->left = NULL;
        ret_node->right = NULL;
        ret_node->op_code = NONE;
    }
    else if (arg->type == IDENT)
    {
        //ret_node = malloc(sizeof(DagNode));
        //ret_node->op_code = ASSIGN;
        //ret_node->is_id = true;
        //ret_node->ids = NULL;

        // Check where the most recent copy of it is
        ret_node = get_most_recent_or_new(most_recents, arg);

        //ret_node->children = g_list_prepend(NULL, recent_node);
    }
    else { assert(false); }

    return ret_node;
}

// Checks to see if this arg remains valid for the given dag node
bool arg_is_valid(Arg *arg, DagNode *node, GHashTable *most_recents)
{
    bool valid = true;

    // Constants are always valid
    if (arg->type != CONST)
    {
        DagNode *recent_node = g_hash_table_lookup(most_recents, arg->ident_val->symbol);
        // If it were NULL then it would rely on an uninitialized symbol
        assert(recent_node!=NULL);

        // Make sure that the node that the old instruction hashed to has the 
        // most recent value for the val.
        if (node->left == recent_node || node->right == recent_node)
        {
            valid = false;
        }
    } 
    return valid;
}


// most_recents holds symbols->DAGnodes with most recent value
// dag_table has instruction hashes mapping to nodes
// Once we check to see if one already exists in the dag table we then need to
// check most_recents to make sure the children are still most recent.
DagNode *dag_unary(GHashTable *most_recents, GHashTable* dag_table, Instruction* instr)
{
    // Check to see if this instruction has already been made
    char *instr_hash = hash_instr(instr);
    DagNode *prev_node = g_hash_table_lookup(dag_table, instr_hash);

    if (prev_node != NULL)
    {
        if (arg_is_valid(instr->arg1, prev_node, most_recents))
        {
            prev_node->ids = g_list_prepend(prev_node->ids, instr->result);
            return prev_node;   
        }
    }
    
    DagNode *arg1_node = get_arg_node(most_recents, dag_table, instr->arg1);
    arg1_node->ids = g_list_remove(arg1_node->ids, instr->result);

    DagNode *new_node = malloc(sizeof(DagNode));
    new_node->visited = false;
    new_node->left = arg1_node;
    new_node->right = NULL;

    new_node->ids = g_list_prepend(NULL, instr->result);
    new_node->op_code = instr->op_code;
    new_node->is_id = true;
    new_node->init_arg = NULL;

    // The result id is now the most recent at this node
    g_hash_table_insert(most_recents, instr->result->symbol, new_node);
    // Also add it to the dag_table, so if it happens again we can find it!
    g_hash_table_insert(dag_table, instr_hash, new_node);

    return new_node;
}



DagNode *dag_binary(GHashTable *most_recents, GHashTable* dag_table, Instruction* instr)
{
    // Check to see if this instruction has already been made
    char *instr_hash = hash_instr(instr);
    DagNode *prev_node = g_hash_table_lookup(dag_table, instr_hash);

    if (prev_node != NULL)
    {
        bool arg1_valid = arg_is_valid(instr->arg1, prev_node, most_recents);
        bool arg2_valid = arg_is_valid(instr->arg2, prev_node, most_recents);

        if (arg1_valid && arg2_valid)
        {
            prev_node->ids = g_list_prepend(prev_node->ids, instr->result);
            return prev_node;   
        }
    }
    
    DagNode *arg1_node = get_arg_node(most_recents, dag_table, instr->arg1);
    DagNode *arg2_node = get_arg_node(most_recents, dag_table, instr->arg2);

    arg1_node->ids = g_list_remove(arg1_node->ids, instr->result);
    arg2_node->ids = g_list_remove(arg2_node->ids, instr->result);

    DagNode *new_node = malloc(sizeof(DagNode));
    new_node->visited = false;
    new_node->left = arg1_node;
    new_node->right = arg2_node;
    new_node->is_id = true;
    new_node->op_code = instr->op_code;
    new_node->init_arg = NULL;
    new_node->ids = g_list_prepend(NULL, instr->result);

    // The result id is now the most recent at this node
    g_hash_table_insert(most_recents, instr->result->symbol, new_node);
    // Also add it to the dag_table, so if it happens again we can find it!
    g_hash_table_insert(dag_table, instr_hash, new_node);

    return new_node;
}

DagBlock *generate_dag(BasicBlock *block)
{
    DagBlock *db = malloc(sizeof(DagBlock));
    // Might be NULL but that doesn't matter
    db->init_label = ((Instruction*)block->instrs->data)->label;

    // Go through and generate initial NULL nodes for everything that the values
    // initially depend on.
    GList *instrs = block->instrs;

    GHashTable *most_recent_table = g_hash_table_new_full(g_str_hash,
                                                    g_str_equal,
                                                    NULL,
                                                    NULL);

    // Don't want to delete values (they're also in most_recent_table)
    // but do want to delete all of our hashed keys.
    GHashTable *dag_table = g_hash_table_new_full(g_str_hash,
                                                    g_str_equal,
                                                    g_free,
                                                    NULL);
    bool test_goto_last = true;
    for (; instrs!=NULL; instrs=instrs->next)
    {
        assert(test_goto_last);
        Instruction *cur_instr = instrs->data;
        // Unary, only look at first arg
        if (cur_instr->op_code == ASSIGN || cur_instr->op_code == PRINT)
        {
            DagNode *assign_node = dag_unary(most_recent_table, dag_table, cur_instr);
        }
        else if (!is_relative_op(cur_instr->op_code) && cur_instr->op_code != GOTO)
        {
            DagNode *bin_node = dag_binary(most_recent_table, dag_table, cur_instr);
        }
        // Otherwise it's a GOTO, no Node required as it's the last one
        else
        {
            test_goto_last = false;
            db->exit_instr = cur_instr;
        }
    }

    db->root_nodes = g_hash_table_get_values(most_recent_table);

    // Only intermediate structures in getting us to our root nodes.
    g_hash_table_destroy(most_recent_table);
    g_hash_table_destroy(dag_table);

    return db;
}

// Compile into a list of Instruction* objects (instr_list)
// Return a list of Arg* for the level above to use
Arg *traverse_dag(DagNode *root, GList** instr_list)
{
    // Special case for init Dag node
    if (root->op_code == NONE)
    {
        // Just return the init'd arg.
        return root->init_arg;
    }
    else if (root->op_code == ASSIGN)
    {
        // Grab the first arg
        Arg *arg = traverse_dag(root->left, instr_list);
        // We need to store it in a result somewhere.
        // Get a temp node!
        if (root->ids == NULL)
        {
            Identifier *temp = get_temp_symbol();
            printf("Temp?? %s\n", temp->symbol);
            Instruction *assign = init_assign_instr(arg, temp);
            Arg *ret_arg = init_arg(IDENT, temp); 
            *instr_list = g_list_prepend(*instr_list, assign);
            return ret_arg;
        }
        else
        {
            // For now assign all variables correctly
            GList *cur_id = root->ids;
            Identifier *to_assign;
            // And now set all the next ones to the first assign result
            for (; cur_id!=NULL; cur_id=cur_id->next)
            {
                to_assign = cur_id->data;
                Instruction *assign = init_assign_instr(arg, to_assign);
                *instr_list = g_list_prepend(*instr_list, assign);
            }
            // Won't be unit'd because ids!=NULL
            Arg *ret_arg = init_arg(IDENT, to_assign);
            return ret_arg;
            /* So here's why it doesn't matter which one we pick:
                THEY HAVE THE SAME VALUE!
                Because we can't change things in place at the moment it really
                doesn't matter.
            */
        }
    }
    // Kind of a hack to get this functionish thing working
    else if (root->op_code == PRINT)
    {
        Arg *arg = traverse_dag(root->left, instr_list);
        Instruction *instr = init_instr(PRINT, arg, NULL, NULL);
        *instr_list = g_list_prepend(*instr_list, instr);
        return arg;
    }
    else if (!is_relative_op(root->op_code))
    {
        assert(false);
    }
    assert(false);
}

GList *compile_dag(BasicBlock *block, DagBlock *dag)
{   
    GList *roots = dag->root_nodes;
    GList *instrs = NULL; 
    for (; roots!=NULL; roots=roots->next)
    {
        traverse_dag(roots->data, &instrs);
    }
    //instrs = g_list_prepend(
    return instrs;
}

// Resets all visited status's on a list of DagNodes <is_visited>
void set_dags_visit_rec(DagNode* root, bool is_visited)
{
    if (root != NULL)
    {
        root->visited = is_visited;
        set_dags_visit_rec(root->left, is_visited);
        set_dags_visit_rec(root->right, is_visited);
    }
}

void set_dags_visit(GList* root_nodes, bool is_visited)
{
    GList *cur= root_nodes;
    for (; cur!=NULL; cur=cur->next)
    {
        printf("\tSetting a dag visit %p\n", cur);
        set_dags_visit_rec((DagNode*)cur->data, is_visited);
    }
}

char *repr_dag_node(DagNode *node)
{
    char *repr = malloc(30);
    sprintf(repr, "OP: %s at %p\n", OP_CODE_REPRS[node->op_code], node);
    return repr;
}

void print_dag_rec(DagNode *cur_node, int depth)
{
    /*
    char *spacing = malloc(depth+1);
    // Dumb as hell but that's how it's got to go
    for (int i=0; i<depth; i++)
    {
        spacing[i] = " ";
    }
    */
    if (cur_node!=NULL && !cur_node->visited)
    {
        cur_node->visited = true;
        char *node_repr = repr_dag_node(cur_node);
        char *format = malloc(strlen(node_repr)+depth+5);
        //sprintf(format, "%s%s\n", spacing,node_repr);
        sprintf(format, "%d%s\n", depth,node_repr);
        printf(format);
        print_dag_rec(cur_node->left, depth+1);
        print_dag_rec(cur_node->right, depth+1);
        //free(spacing);
        free(node_repr);
        free(format);
    }
}

// Recursively traces the DAG from the root nodes
void print_dag(DagBlock *dag)
{
    GList *cur_root = dag->root_nodes;

    for (; cur_root!=NULL; cur_root=cur_root->next)
    {
        // Print everything
        set_dags_visit_rec(cur_root->data, false);
        print_dag_rec(cur_root->data, 0);
        // Reset anything we just did
        set_dags_visit_rec(cur_root->data, false);
    }
}

void print_blocks(int num_instrs, GList *block_list)
{
    for (; block_list!=NULL; block_list=block_list->next)
    {
        BasicBlock *block = block_list->data;
        GList *instr = block->instrs;
        printf("------------START BASIC BLOCK ------------\n");
        for (; instr!=NULL; instr=instr->next)
        {
            print_instr((Instruction*)instr->data); 
        }
        printf("------------------------------------------\n");
    }
}

void calc_next_use(BasicBlock *b)
{

}

void calc_next_use_all(GList *blocks)
{
    GList *cur = blocks;
    for (; cur!=NULL; cur=cur->next)
    {
        BasicBlock *b = cur->data;
        calc_next_use(b);
    }
}


void combine_blocks(GList *block_list, GPtrArray **instr_list, int *num_values)
{
    GList *head = block_list;
    *num_values = 0;
    *instr_list = g_ptr_array_new();
    for (; head!=NULL; head=head->next)
    {
        GList *instr = ((BasicBlock*)head->data)->instrs;
        for (; instr!=NULL; instr=instr->next)
        {
            g_ptr_array_add(*instr_list,(Instruction*)instr->data);
            *num_values += 1;
        }
    }
}

// Not super efficient but fine for now
BasicBlock *get_addr_block(GList *blocks, Instruction *to_find)
{
   GList *head = blocks;
   for (; head!=NULL; head=head->next)
    {
        BasicBlock *cur_block = head->data;
        GList *instr = cur_block->instrs;
        for (; instr!=NULL; instr=instr->next)
        {

            if (((Instruction*)instr->data) == to_find)
            {
                return cur_block;
            }
        }
    }
    // Should always be in a block
    assert(false);
    return NULL;
}

// Adds successor ptrs in place to the list of BasicBlocks
void add_successors(GList *blocks)
{
    GList *head = blocks;
    while (head!=NULL)
    {
        BasicBlock *cur_block = head->data;
        cur_block->successors = NULL;
        GList *cur_data = cur_block->instrs;
        for (; cur_data!=NULL; cur_data=cur_data->next)
        {
            Instruction *instr = cur_data->data;
            if (instr->goto_addr != NULL)
            {
                BasicBlock *new_succ = get_addr_block(blocks, instr->goto_addr);
                cur_block->successors = g_list_prepend(cur_block->successors, new_succ);
            }
        }
        head=head->next;
        if (head != NULL)
        {
            BasicBlock *next_block = head->data;
            cur_block->successors = g_list_prepend(cur_block->successors, next_block);
        }
    }
}

/* Construct initial basic blocks from the given instruction list */
GList *make_blocks(GPtrArray *instr_list, int num_instrs)
{
    if (num_instrs == 0) { return NULL; }

    GList *block_list = NULL;
    // First 3-addr ins is a leader
    BasicBlock *cur_block = malloc(sizeof(BasicBlock)); 
    cur_block->instrs = NULL;

    Instruction *cur_instr, *last_instr;
    last_instr = get_instr(instr_list, num_instrs, 0);
    cur_block->instrs = g_list_prepend(cur_block->instrs, last_instr);
    for (int i=1; i<num_instrs; i++)
    {
        cur_instr = get_instr(instr_list, num_instrs, i);

        bool start_new_block = false;
        // Any inst with a label is a leader
        if (cur_instr->label != NULL) { start_new_block = true; }
        // any instr that immediately follows a jmp is a leader
        if (last_instr->op_code == GOTO || is_relative_op(last_instr->op_code))
        {
            start_new_block = true;
        }

        if (start_new_block)
        {
            cur_block->instrs = g_list_reverse(cur_block->instrs);
            // Prepending is more efficient
            block_list = g_list_prepend(block_list, cur_block);
            cur_block = malloc(sizeof(BasicBlock));
            cur_block->instrs = NULL;
            cur_block->instrs = g_list_prepend(cur_block->instrs, cur_instr);
        }
        else
        {
            cur_block->instrs = g_list_prepend(cur_block->instrs, cur_instr);
        }

        last_instr = cur_instr;
    }
    cur_block->instrs = g_list_reverse(cur_block->instrs);
    block_list = g_list_prepend(block_list, cur_block);
    block_list = g_list_reverse(block_list);
    add_successors(block_list);

    return block_list;
}

