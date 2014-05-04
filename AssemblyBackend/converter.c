#include "converter.h"

// Define constants for register names
//static const char *EBP_REGISTER = "ebp"; // Base (code) pointer reg
//static const char *ESP_REGISTER = "esp"; // Stack pointer reg
Register *EBP_REGISTER;
Register *ESP_REGISTER;
FILE *out_file;

// Instructions
static const char *MOVE_INSTR = "mov";
static const char *PUSH_INSTR = "push";
static const char *SUB_INSTR = "sub";
static const char *ADD_INSTR = "add";
static const char *MULT_INSTR = "imul";

// Sizes
static const int CHAR_SIZE = 1;
static const int INT_SIZE = 4;
static const int LONG_SIZE = 4;
static const int DOUBLE_SIZE = 8;

// Options
static const char *DWORD_OPTION = "DWORD";

Register *registers[NUM_REGISTERS];
int cur_stack_offset = 0;
// EAX, EBX, ECX, EDX, ESI, EDI

// Init to start addresses
int STACK_ADDR;
int CODE_ADDR;

int get_byte_size(eType type)
{
    int size;
    switch (type)
    {
        case INTEGER:
            size = INT_SIZE;
            break;
        case CHAR:
            size = CHAR_SIZE;
            break;
        case DOUBLE:
            size = DOUBLE_SIZE;
            break;
        case LONG:
            size = LONG_SIZE;
            break;
        default:
            printf("COMPILE ERROR: Unknown Type");
            size = -1;
            break;
    }
    return size;
}

void write_1instr(const char *command, char *arg)
{
    fprintf(out_file, "\t%s %s\n", command, arg); 
}

void write_2instr_with_option(const char *command, const char *option,
                              char *arg1, char *arg2)
{


    fprintf(out_file, "\t%s %s %s, %s\n", command, option, arg1, arg2); 
}

void write_2instr(const char *command,
                  char *arg1, char *arg2)
{
    write_2instr_with_option(command, "", arg1, arg2);
}

char *addr_ind(char *reg)
{
    char *repr = malloc((strlen(reg) + 3) * sizeof(char));
    sprintf(repr, "[%s]", reg);
    return repr;
}

char *addr_mult(char *reg, int fact)
{
    // Assuming fact are less than 1,000 in this case
    char *repr = malloc(sizeof(strlen(reg)) + 5);
    sprintf(repr,"%s*%d", reg, fact);
    return repr;
}

char *addr_add(char *reg, int offset)
{
    // Assuming offsets are less than 1,000 in this case
    char *repr = malloc(sizeof(strlen(reg)) + 5);
    if (offset < 0) { sprintf(repr,"%s-%d", reg, offset); }
    else if (offset > 0) { sprintf(repr,"%s+%d", reg, offset); }
    else { sprintf(repr, "%s", reg); }

    return repr;
}

char *char_int(int x) 
{
    char *repr = malloc(sizeof(char)*10);
    sprintf(repr, "%d", x);
    return repr;
}

char *char_double(double x) 
{
    char *repr = malloc(sizeof(char)*10);
    sprintf(repr, "%f", x);
    return repr;
}

char *repr_const(Constant *c)
{
    char *repr;

    switch (c->type)
    {
        case INTEGER:
            repr = char_int(c->int_val);
            break;
        case DOUBLE:
            repr = char_double(c->float_val);
            break;
        case CHAR:
            repr = c->str_val;
            break;
        case LONG:
            // TODO: give long's their own method
            repr = char_int(c->int_val); 
            break;
        default:
            printf("CHAR_CONSTANT ERROR: unknown type");
            repr = "UH OH";
            break;
    }
    return repr;
}


/**
 *  Make room for local variables
 */
void init_stack_variables(GHashTable *address_table)
{
    //TODO: change this to local identifiers eventually
    GList *local_ids = get_all_identifiers(address_table);
    GList *head = local_ids;

    // Just push uninitialized space onto the stack
    cur_stack_offset = 0;
    int data_size = 0;
    Identifier *cur_id;
    char *esp_repr = repr_reg(registers[0]);
    for (; local_ids != NULL; local_ids = local_ids->next)
    {
        cur_id = (Identifier *) local_ids->data;
        data_size = get_byte_size(cur_id->type);

        // Doesn't matter what we push on.
        write_1instr(PUSH_INSTR, esp_repr);
        cur_stack_offset += data_size;
        printf("VARIABLE %s AT OFFSET %d\n", cur_id->symbol, cur_stack_offset);

        // No value when initialized
        cur_id->on_stack = false;
        cur_id->offset = cur_stack_offset;
        cur_id->address_descriptor = NULL;
        
        //Address *addr = malloc(sizeof(Address));
        //addr->type = STACK_TYPE;
        //addr->stack_offset_val = offset;
        //cur_id->address_descriptor = g_list_prepend(cur_id->address_descriptor, addr);
    }
    free(esp_repr);
    // Now free the list structure itself (not FULL as we still want the Identifiers)
    g_list_free(head);
}

// TODO: Better place to put these?
void init_registers()
{
    ESP_REGISTER = malloc(sizeof(Register));
    ESP_REGISTER->repr = "esp";
    ESP_REGISTER->variables_held = NULL;

    EBP_REGISTER = malloc(sizeof(Register));
    EBP_REGISTER->repr = "ebp";
    EBP_REGISTER->variables_held = NULL;

    for (int i=0; i < NUM_REGISTERS; i++)
    {
        registers[i] = malloc(sizeof(Register));
        // As specified in Docs, an empty Glist* is NULL
        registers[i]->variables_held = NULL;
    }

    // Manually give them reprs
    registers[0]->repr = "eax";
    registers[1]->repr = "ebx";
    registers[2]->repr = "ecx";
    registers[3]->repr = "edx";
    registers[4]->repr = "esi";
    registers[5]->repr = "edi";
}


const char *repr_op_code(eOPCode op_code)
{
    const char *const_repr;
    switch (op_code)
    {
        case ASSIGN:
            const_repr = MOVE_INSTR;
            break;
        case ADD:
            const_repr = ADD_INSTR;
            break;
        case SUB:
            const_repr = SUB_INSTR;
            break;
        case MULT:
            const_repr = MULT_INSTR;
            break;
        default:
            const_repr = "NOT IMPLEMENTED";
            break;
    }
    return const_repr;
}


/* Clears all variables except <reserved_id> stored in the passed <reg> 
   Handy to dump everything except one variable so that we don't do 
   unnecessary dumps on saves. Pass NULL for no removal. */
void dump_reg_with_reserve(Register *reg, Identifier *reserved_id)
{
    GList *cur_list = reg->variables_held;
    Identifier *cur_id;
    for (; cur_list!=NULL; cur_list=cur_list->next)
    {
        cur_id = cur_list->data;
        // Don't remove the reserved_id
        if (cur_id == reserved_id)
        {

            // Find out where to store it

            // 1 - See if it exists elsewhere
            // Shouldn't be NULL... it's already here!
            assert(cur_id->address_descriptor != NULL);

            if (cur_id->address_descriptor->next != NULL)
            {
                // If it does then we can remove it from here
                cur_id->address_descriptor = remove_reg_from_addrs(cur_id->address_descriptor, reg);
            }
            else
            {
                // 2 - If this is the only place, store it at its designated place in the stack
                char *esp_repr = repr_reg(ESP_REGISTER);
                char *added_repr= addr_add(esp_repr, cur_id->offset);
                char *result_loc = addr_ind(added_repr);
                char *rrepr = repr_reg(reg);
                write_2instr(MOVE_INSTR, result_loc, rrepr);

                free(esp_repr);
                free(added_repr);
                free(result_loc);
                free(rrepr);

                // And tell the identifier it's no longer stored in the reg.
                // Note: don't tell register id isn't there because we clear it in a second.
                cur_id->address_descriptor = remove_reg_from_addrs(cur_id->address_descriptor, reg);

                // and it now has the correct value on the stack
                cur_id->on_stack = true;
            }
        }
    }

    // The register now holds nothing! 
    g_list_free(reg->variables_held);
    reg->variables_held = NULL;

    //... except if we wanted to reserve a variable
    if (reserved_id != NULL)
    {
        reg->variables_held = g_list_prepend(reg->variables_held, reserved_id);
    }
}

/* Clears all variables stored in the passed <reg> */
void dump_reg(Register *reg)
{
    dump_reg_with_reserve(reg, NULL);
}



// For ids being used as arguments in operations
// From pg. 547 - Not the most efficient but hopefully readable
// We need all three args in order to choose the correct register
// reserved registers cannot be used for this argument
// Returns reg but it needs to be dumped later!
Register *get_reg_for_arg(Identifier *arg_id, Identifier *result_id, 
                          Identifier *other_id, bool *reserved)
{
    // If arg_id is is already in a register, pick it
    GList *addrs = arg_id->address_descriptor;
    Register *cur_addr; 
    for(; addrs != NULL; addrs = addrs->next)
    {
        return (Register*) addrs->data;
    }

    // Check to see if there's an empty register
    for (int i=0; i<NUM_REGISTERS; i++)
    {
        if (registers[i]->variables_held == NULL)
        {
            return registers[i];
        }
    }

    
    // id isn't currently in a register and there are no empty registers
    // Find out what values we need to store to open one up

    // will determine which register to dump
    int register_scores[NUM_REGISTERS] = {0};


    // Determine a score for each register based on the number of stores
    // we'll have to do
    for (int i=0; i<NUM_REGISTERS; i++)
    {
        // Don't bother doing all the extra work if the register is reserved
        if (!reserved[i])
        {
            GList *cur_list = registers[i]->variables_held;
            Identifier *cur_id;
            for (; cur_list!=NULL; cur_list=cur_list->next)
            {
                cur_id = cur_list->data;
                
                // If we are overwriting cur_id (e.g. cur_id = a + b)
                // then don't increment score, unless we do (cur_id = cur_id + b)
                if (cur_id == result_id && cur_id != other_id)
                {
                    continue;
                }

                // If 1, only stored here. Have to store it.
                if (g_list_length(cur_id->address_descriptor) == 1)
                {
                    register_scores[i]++;
                }

                // DIDN'T IMPLEMENT FROM BOOK: If cur_id is no longer used in future
                // instructions we can ignore it
            }
        }
    }

    int min_score = 9999999;
    Register *best_reg = NULL;
    // Grab the lowest scores and register
    for (int i=0; i<NUM_REGISTERS; i++)
    {
        if (!reserved[i] &&
            register_scores[i] < min_score)
        {
            min_score = register_scores[i];
            best_reg = registers[i];
        }
    }

    return best_reg;
}

int get_index_for_reg(Register *reg)
{
    for (int i=0; i<NUM_REGISTERS; i++)
    {
        if (reg == registers[i])
        {
            return i;
        }
    }
    return -1;
}

// Assuming for now that everything passed here will be ids
void *get_regs(Instruction *instr, Register **res_reg, 
               Register **arg1_reg, Register **arg2_reg)
{
    bool reserved_regs[6] = {0};
    // Figure out which variables are needed
    // For now arg2 should ALWAYS be null
    // But in the future that might not be true
    if (instr->op_code == ASSIGN && instr->arg2 == NULL)
    {
        // Use book algorithm
        if (instr->arg1->type == IDENT)
        {
            // This is easy since we can just use the first args register
            Register *arg_reg = get_reg_for_arg(instr->arg1->ident_val, instr->result, 
                                                NULL, reserved_regs);
            *arg1_reg = arg_reg;
            *res_reg = arg_reg;
        }
        // Even if we just assign a constant we need to find a register for the result
        else
        {
            //TODO: additional rules for this probably
            *res_reg = get_reg_for_arg(instr->result, NULL, NULL, reserved_regs);
        }
    }
    else
    {
        // Only want to get registers for non-constants
        if (instr->arg1->type == IDENT)
        {
            // In this case we need to find registers for all of the them
            *arg1_reg = get_reg_for_arg(instr->arg1->ident_val, 
                              instr->result, instr->arg2->ident_val, reserved_regs);

            reserved_regs[get_index_for_reg(*arg1_reg)] = true;
        }

        if (instr->arg2->type == IDENT)
        {
            *arg2_reg = get_reg_for_arg(instr->arg2->ident_val, 
                              instr->result, instr->arg1->ident_val, reserved_regs);

            reserved_regs[get_index_for_reg(*arg2_reg)] = true;
        }
 
        //TODO: there are extra rules for this (pg. 548)
        *res_reg = get_reg_for_arg(instr->result, NULL, NULL, reserved_regs);

    }
}

// Just get it's address for now, if they want what's 
// in it the caller can indirect it
char *repr_ident(Identifier *ident)
{
    char *esp_repr = repr_reg(ESP_REGISTER);
    char *repr = addr_add(esp_repr, ident->offset);
    free(esp_repr);
    return repr;
}


// Returns the character representation for the arg.
// If it's a constant it's just the constant
// If it's an identifier return a reg if possible, otherwise the indirect
// value of the stack location.
// Either way it needs to be freed.
char *repr_arg(Arg *arg)
{
    char *repr;
    // Constant is pretty simple
    if (arg->type == CONST)
    {
        repr = repr_const(arg->const_val);
    }
    else if (arg->type == IDENT)
    {
        Identifier *id = arg->ident_val;
        // Check to see if it's already in a register - more efficient!
        // Only one of these will be used.
        Register *reg_loc = NULL; 
        GList *arg_addrs = id->address_descriptor;
        for (; arg_addrs!=NULL; arg_addrs=arg_addrs->next)
        {
            reg_loc = arg_addrs->data;
            break;
        }

        // Grab it from the register
        if (reg_loc != NULL)
        {
            repr = repr_reg(reg_loc);
        }
        // If the current value held in the stack is current then we're fine
        else if (arg->ident_val->on_stack)
        {
            char *stack_base = repr_reg(ESP_REGISTER);
            char *with_offset = addr_add(stack_base, id->offset);
            repr = addr_ind(with_offset);
            free(stack_base);
            free(with_offset);
        }
        // Indicates the variable is neither on the stack nor
        // in a register -> has no representation
        else { assert(false); }
    }
    else { assert(false); }
    return repr;
}


/* Ensures that id is in the load_reg, either by loading it or just validating
   that it's already there. If the id has never been used (i.e. not in a register
   nor is it valid on the stack, just clear out the register, and tell the register
   and variable they are in each other (EVEN WHEN UNINITIALIZED!!!).  */
void ensure_register(Identifier *id, Register* load_reg)
{
    // Check to see if it's already loaded
    GList *arg_addrs = id->address_descriptor;
    Register *alt_reg = NULL;
    for (; arg_addrs!=NULL; arg_addrs=arg_addrs->next)
    {
        if (arg_addrs->data == load_reg)
        {
            return;
        }
        else
        {
            alt_reg = arg_addrs->data;
        }
    }
   
    // If it isn't then we need to issue a load command
    // Before we issue load first adjust things with registers
    dump_reg(load_reg);
    // Try moving it from a different register
    char *load_from = NULL;
    if (alt_reg != NULL)
    {
        // Keeping for clarity: This can happen when we reserve a register that we 
        // didn't explicitly request for this particular id. For example when 
        // we do a copy in anticipation of something like int x=5; t0=x+5;
        // In this case x should be copied and then we add 5. So we request
        // a register for t0 but then ensure x into it.

        // If this happens then we called ensure register on a register
        // that didn't contain the value, but another did. This seems to indicate
        // something is going wrong with get_reg.
        //printf("LONG RUNNING THING: I DON'T THINK THIS SHOULD HAPPEN\n");

        load_from = repr_reg(alt_reg);
    }
    // If it's not initialized at all, then clearing the register above is enough.
    else if (id->on_stack)
    {
        printf("Loading from stack\n");
        // Now for the actual loading
        char *load_from_direct = repr_ident(id);            
        // Want the value, not the address
        load_from = addr_ind(load_from_direct);
        free(load_from_direct);
    }

    // Tell the register its holding the variable
    load_reg->variables_held = g_list_prepend(load_reg->variables_held, id);
    // Tell the variable it's also in register
    id->address_descriptor = g_list_prepend(id->address_descriptor, load_reg);

    // If load_from is NULL then the variable was formally unitialized so 
    // there's nowhere to load it from. Dumping, and ensuring they're correct
    // addr details is enough.
    if (load_from != NULL)
    {
        char *load_to = repr_reg(load_reg);       
        write_2instr(MOVE_INSTR, load_to, load_from);
        free(load_from);
        free(load_to);
    }
}

// TODO:Need to get a better name for this
// load_reg must either have the arg in it or it must be empty!
char *basic_handle_arg(Arg *arg, Register *load_reg)
{
    char *repr;
    if (arg->type == IDENT)
    {
    }
    else if (arg->type == CONST)
    {
        repr = repr_const(arg->const_val);
    }
    else { assert(false); }
    return repr;
}


void compile_unary(Instruction *instr)
{
    switch (instr->op_code)
    {
        case ASSIGN:;
            // The second argument is NULL (for now) 
            Arg *operand = instr->arg1;
            Identifier *assigned = instr->result;
            // No reserved regs in a unary assign
            bool reserved[NUM_REGISTERS] = {0};

            // If it's constant then we need to find a register 
            // for the result 
            if (operand->type == CONST)
            {
                // Variable's value is now only here.
                Register *result_reg = get_reg_for_arg(assigned, assigned,
                                                       NULL, reserved);
                ensure_register(assigned, result_reg);
                // Clear out everything from the register except the assignment
                // we just made
                dump_reg_with_reserve(result_reg, assigned);
                // Now tell the variable it's only place is in the register
                g_list_free(assigned->address_descriptor);
                assigned->address_descriptor = NULL;
                assigned->address_descriptor = g_list_prepend(
                                                    assigned->address_descriptor, 
                                                    result_reg);
                assigned->on_stack = false;

                char *result_repr = repr_reg(result_reg);
                char *const_repr = repr_const(operand->const_val);
                // And finally do the load
                write_2instr(MOVE_INSTR, result_repr, const_repr);
                free(result_repr);
                free(const_repr);
            }
            else if (operand->type == IDENT)
            {
                Identifier *new_id = operand->ident_val;
                // Make sure the OPERAND has a register!
                Register *operand_reg = get_reg_for_arg(new_id, new_id, 
                                                        NULL, reserved);
                ensure_register(new_id, operand_reg);
                // No need to dump as we aren't changing any value
                // Just add the result to this operand's register. Tell the id
                // that it's been assigned here
                operand_reg->variables_held = g_list_prepend(
                                                operand_reg->variables_held,
                                                assigned);
                GList *cur_addrs = assigned->address_descriptor;
                assigned->on_stack = false;
                g_list_free(assigned->address_descriptor);
                assigned->address_descriptor = NULL;
                assigned->address_descriptor = g_list_prepend(
                                                    assigned->address_descriptor, 
                                                    operand_reg);
                // Note: no direct load instr needed
            }
            else { assert(false); }
            break;
        default:
            // Can't happen at the moment
            assert(false);
            break;
    }
}

void basic_compile(GPtrArray *instr_list, GHashTable* symbol_table,
                   int num_instrs)
{   
    for (int i=0; i<num_instrs; i++)
    {
        // Check to see if either of the arguments is the result symbol
        Instruction *cur_instr = get_instr(instr_list, num_instrs, i);

        // Handle assign seperately
        if (cur_instr->op_code == ASSIGN)
        {
            printf("UNARY\n");
            compile_unary(cur_instr); 
            continue;
        }

        Identifier *result = cur_instr->result;
        // -1->can't reuse result reg, 0->arg1==result, 1->arg2==result
        int reg_to_add = -1;

        assert(cur_instr->arg1 != NULL);
        if (cur_instr->arg1->type == IDENT && 
            cur_instr->arg1->ident_val == result)
        {
            reg_to_add = 0;
        }

        // At the moment ASSIGN is the only unary operator and it's handled above.
        assert(cur_instr->arg2 != NULL);
        // the second arg could be NULL (assignment operator)
        if (reg_to_add != -1
            && cur_instr->arg2->type == IDENT 
            && cur_instr->arg2->ident_val == result)
        {
            reg_to_add = 1;
        }

        bool reserved_regs[6] = {0};
        // Get a register for the result
        Register *result_reg = get_reg_for_arg(result, result, 
                                               NULL, reserved_regs);
        ensure_register(cur_instr->result, result_reg);

        //print_registers(registers, NUM_REGISTERS);
        reserved_regs[get_index_for_reg(result_reg)] = true;

        // Repr of the arg which will opped to the result reg.
        char *op_arg_repr;
        char *result_repr = repr_reg(result_reg);
        switch (reg_to_add)
        {
            case -1:;
                printf("CASE -1\n");
                // Load the first arg into the result reg and then op the 
                // other register

                // If it's a constant then we have to load it with the correct 
                // option
                if (cur_instr->arg1->type == CONST)
                {
                    // Free up the register so we can throw the constant in
                    dump_reg(result_reg);
                    char *const_arg = repr_const(cur_instr->arg1->const_val);
                    write_2instr_with_option(MOVE_INSTR, DWORD_OPTION, 
                                             result_repr, const_arg);
                    free(const_arg);
                }
                else
                {
                    // This does a bit of redundant work but it should be fine
                    // Load the operand into the result reg. (something will
                    // be added/mult'd/etc. soon
                    // Ensure does any loading we might need to do.
                    ensure_register(cur_instr->arg1->ident_val, result_reg);
                    /*
                    printf("After reserving for first argument\n");
                    fprintf(fp, "AFTER ENSURE\n");
                    char *ident_arg = repr_ident(cur_instr->arg1->ident_val);
                    char *indirect = addr_ind(ident_arg);
                    write_2instr(fp, MOVE_INSTR, result_repr, indirect);
                    fprintf(fp, "AFTER MOVE\n");
                    free(ident_arg);
                    free(indirect);
                    */
                }

                op_arg_repr = repr_arg(cur_instr->arg2);
                break;
            case 0:
                printf("CASE 0\n");
                // The first argument is already in result reg just add second
                op_arg_repr = repr_arg(cur_instr->arg2);
                break;
            case 1:
                printf("CASE 1\n");
                // The second argument is already in result reg just add first
                op_arg_repr = repr_arg(cur_instr->arg1);
                break;
            default:
                assert(false);
        }
        
        // Fix result reg:
        // 1) Update result's reg so that it now only contains the result id 
        // 2) Clear everything from result's descriptor, invalidate memory loc
        // 3) Remove this register from every other variable 

        // (1) & (3)
        dump_reg_with_reserve(result_reg, result);
        // Now done in reserve
        //result_reg->variables_held = g_list_prepend(result_reg->variables_held, result);
        // (2)
        g_list_free(result->address_descriptor); 
        result->address_descriptor = g_list_prepend(result->address_descriptor, result_reg);
        result->on_stack = false;

        write_2instr(repr_op_code(cur_instr->op_code), result_repr,
                     op_arg_repr);
    }
}

char *get_assem_arg_repr(Arg *arg)
{
    assert(arg != NULL); 

    char *repr;
    switch (arg->type)
    {
        case CONST:
            repr = repr_const(arg->const_val);
            break;
        case IDENT:
            repr = repr_ident(arg->ident_val);
            break;
        default:
            // Should never happen
            assert(false);
            break;
    }
    return repr;
}



/* Naive way of compiling. Everything is loaded, operated on, then retured to 
    its proper stack location */
void stack_compile(GPtrArray *instr_list, GHashTable* symbol_table,
                   int num_instrs)
{
    for (int i=0; i<num_instrs; i++)
    {
        Instruction *instr = get_instr(instr_list, num_instrs, i);
        if (instr->op_code == ASSIGN)
        {
            // Simple, just dump the first argument into the stack location 
            // of the result

            // Need to be careful with string functions so we can clean it all back up
            char *esp_repr = repr_reg(ESP_REGISTER);
            char *added_repr= addr_add(esp_repr, instr->result->offset);
            char *result_loc = addr_ind(added_repr);
            // Take care of intermediate strings
            free(esp_repr);
            free(added_repr);


            char *arg_repr;
            bool requires_option = false;
            // In an assignment only the first argument is populated
            switch(instr->arg1->type)
            {
                case CONST:
                    // Nice and easy, just get its string repr
                    arg_repr = repr_const(instr->arg1->const_val);
                    requires_option = true;
                    break;
                case IDENT:;
                    // Note: Move operations cannot be mem->mem
                    // Get addr of its ptr.
                    char *esp_repr = repr_reg(ESP_REGISTER);
                    char *added_repr= addr_add(esp_repr, instr->arg1->ident_val->offset);
                    arg_repr = addr_ind(added_repr);
                    free(esp_repr);
                    free(added_repr);

                    char *intermed_reg_repr = repr_reg(registers[0]);
                    // Then load it
                    write_2instr(MOVE_INSTR, intermed_reg_repr, arg_repr);
                    free(arg_repr);
                    // Now pt the repr to the newly loaded instr
                    arg_repr = intermed_reg_repr;
                    break;
                // I think this should never happen (includes INSTR)
                default:
                    assert(false);
                    break;
            }

            // If the second argument is a constant then we need an option
            if (requires_option)
            {
                write_2instr_with_option(MOVE_INSTR, DWORD_OPTION, result_loc, arg_repr);
            }
            else
            {
                write_2instr(MOVE_INSTR, result_loc, arg_repr);
            }
            // Free the strings we've been manipulating
            free(arg_repr);
            free(result_loc);
        } 
        // Non-assign instructions require more work.
        else
        {
            // Get the first instr and load it into register 0
            char *arg_repr = get_assem_arg_repr(instr->arg1);
            // Because we're doing a move we want whatever is in the memory slot (not the address itself)
            if (instr->arg1->type == IDENT) 
            { 
                char *direct_repr = arg_repr;
                arg_repr = addr_ind(arg_repr); 
                free(direct_repr);
            }
            // Save once since we use it 3 times in this block
            char *esp_repr = repr_reg(registers[0]);
            write_2instr(MOVE_INSTR, esp_repr, arg_repr);
            free(arg_repr);
    
            // Now get the second arguments repr and do the operation
            arg_repr = get_assem_arg_repr(instr->arg2);
            if (instr->arg2->type == IDENT)
            {
                char *direct_repr = arg_repr;
                arg_repr = addr_ind(arg_repr); 
                free(direct_repr);
            }

            write_2instr(repr_op_code(instr->op_code), esp_repr, arg_repr);
            free(arg_repr);
 
            // The final step is calculating where to store the result
            char *add_repr = addr_add(esp_repr, instr->result->offset);
            char *result_loc = addr_ind(add_repr); 
            // Again just using register 0 for now
            write_2instr(MOVE_INSTR, result_loc, esp_repr);
            free(add_repr);
            free(result_loc);
            free(esp_repr);
        }
    }
}

void write_header()
{
    fprintf(out_file, "SECTION .text\n");
    fprintf(out_file, "\tglobal _start\n");
    fprintf(out_file, "_start:\n");
}

void write_exit()
{
    // Set exit code 0 - OK
    fprintf(out_file, "\tmov ebx, 0\n");
    // Send the exit command to kernel
    fprintf(out_file, "\tmov eax, 1\n");
    // Use the 0x80 interrrupt to call kernel
    fprintf(out_file, "\tint 0x80\n");

}

void init_file(char *fname)
{
    out_file = fopen(fname, "w+");
}

void close_file()
{
    fclose(out_file);
}

void compile(GPtrArray *instr_list, GHashTable* symbol_table,
             int num_instrs, char *fname)
{
    if (num_instrs <= 0)
    {
        printf("Empty Instruction Set: No output created\n");
        return;
    }

    init_file(fname);
    init_registers();
    write_header();
    init_stack_variables(symbol_table);
    //stack_compile(instr_list, symbol_table, num_instrs, file);
    basic_compile(instr_list, symbol_table, num_instrs);
    write_exit();
    close_file();
    printf("END: Printing Registers\n");
    print_registers(registers, NUM_REGISTERS);
}



