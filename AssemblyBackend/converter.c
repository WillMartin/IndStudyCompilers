#include "converter.h"
/* Contains intermediate to assembly code conversion code. */
// Actually define global

/* Maps types to corresponding byte size. */
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
        case BOOL:
            size = BOOL_SIZE;
            break;
        default:
            printf("COMPILE ERROR: Unknown Type");
            size = -1;
            break;
    }
    return size;
}

void write_label(Instruction *instr)
{
    fprintf(out_file, "%s:\n", instr->label);
}

void write_0instr(const char *command)
{
    fprintf(out_file, "\t%s\n", command); 
}

/* Writes out an instruction that requires an x86 command and one argument */
void write_1instr(const char *command, char *arg)
{
    fprintf(out_file, "\t%s %s\n", command, arg); 
}

/* Writes out an instruction that requires an x86 command, a command (e.g. DWORD)
     and two arguments */
void write_2instr_with_option(const char *command, const char *option,
                              char *arg1, char *arg2)
{
    fprintf(out_file, "\t%s %s %s, %s\n", command, option, arg1, arg2); 
}

/* Writes out an instruction that requires an x86 command and two args */
void write_2instr(const char *command,
                  char *arg1, char *arg2)
{
    write_2instr_with_option(command, "", arg1, arg2);
}

void store_caller_regs()
{
    // The first register is the result and the next 3 are not nec. kept
    // by calling function.
    // Annoyingly hardcoded  
    // Also a bit inefficient as we don't know if we actually need to dump them
    for (int i=0; i<4; i++)
    {
        CUR_STACK_OFFSET += 4;
        Register *cur_reg = REGISTERS[i];
        char *from_repr = repr_reg(cur_reg);
        write_1instr(PUSH_INSTR, from_repr);
        free(from_repr);
    }
}

void restore_caller_regs()
{
    // Also a bit inefficient as we don't know if we actually need to dump them
    char *offset_repr = repr_int(16);
    char *rreg = repr_reg(ESP_REGISTER);
    write_2instr(ADD_INSTR, rreg, offset_repr);
    free(rreg);
    free(offset_repr);
    for (int i=0; i<4; i++)
    {
        CUR_STACK_OFFSET -= 4;
        Register *cur_reg = REGISTERS[i];
        char *from_repr = repr_stack_from_offset(-4 * (i + 1));
        char *to_repr = repr_reg(cur_reg);
        write_2instr(MOVE_INSTR, to_repr, from_repr);
        free(from_repr);
        free(to_repr);
    }
}

// Only doing integers and booleans
void print_variable(Identifier *id)
{
    char const *fmt;
    int id_size;
    switch (id->type)
    {
        case INTEGER:
            fmt = NUM_PRINT;
            id_size = INT_SIZE;
            break;
        case BOOL:
            fmt = NUM_PRINT;
            id_size = BOOL_SIZE;
            break;
        default:
            assert(false); 
            break;
    }

    store_caller_regs();
    //fprintf(out_file, "CHECKING OFFSET %s\n", CUR_STACK_OFFSET);
    char *ident_repr = repr_ident(id);
    fprintf(out_file, "\t%s %s\n", PUSH_INSTR, ident_repr);
    fprintf(out_file, "\t%s %s\n", PUSH_INSTR, fmt);
    fprintf(out_file, "\tcall printf\n");
    char *reg_repr = repr_reg(ESP_REGISTER);
    fprintf(out_file, "\t%s %s, %d\n", ADD_INSTR, reg_repr, id_size + 4);
    free(reg_repr);
    free(ident_repr);
    restore_caller_regs();
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
    CUR_STACK_OFFSET = 0;
    int stack_offset = 0;
    int data_size = 0;
    Identifier *cur_id;
    char *esp_repr = repr_reg(ESP_REGISTER);
    for (; local_ids != NULL; local_ids = local_ids->next)
    {
        cur_id = (Identifier *) local_ids->data;
        data_size = get_byte_size(cur_id->type);
        stack_offset += data_size;

        // No value when initialized
        cur_id->on_stack = false;
        cur_id->offset = stack_offset;
        cur_id->address_descriptor = NULL;
    }
    char *offset_repr = repr_int(stack_offset);
    write_2instr(SUB_INSTR, esp_repr, offset_repr);
    free(esp_repr);
    free(offset_repr);
    // Now free the list structure itself (not FULL as we still want the Identifiers)
    g_list_free(head);
}

/* Initializes all registers with Register structs along with their names */
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
        REGISTERS[i] = malloc(sizeof(Register));
        // As specified in Docs, an empty Glist* is NULL
        REGISTERS[i]->variables_held = NULL;
    }

    // Manually give them reprs
    REGISTERS[0]->repr = "eax";
    REGISTERS[1]->repr = "ebx";
    REGISTERS[2]->repr = "ecx";
    REGISTERS[3]->repr = "edx";
    REGISTERS[4]->repr = "esi";
    REGISTERS[5]->repr = "edi";
}

void move_id_to_stack(char *from_loc, Identifier *id)
{
    char *esp_repr = repr_reg(ESP_REGISTER);
    char *added_repr= repr_addr_add(esp_repr, id->offset);
    char *result_loc = repr_addr_ind(added_repr);
    write_2instr(MOVE_INSTR, result_loc, from_loc);
    free(esp_repr);
    free(added_repr);
    free(result_loc);
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
        if (cur_id != reserved_id)
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
                char *added_repr= repr_addr_add(esp_repr, cur_id->offset);
                char *result_loc = repr_addr_ind(added_repr);
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


/* Finds and returns a register if the ID is current in any,
    else returns NULL */
Register *get_current_reg(Identifier *id)
{
    GList *addrs = id->address_descriptor;
    Register *cur_addr; 
    for(; addrs != NULL; addrs = addrs->next)
    {
        return (Register*) addrs->data;
    }
    return NULL;
}

// For ids being used as arguments in operations
// From pg. 547 - Not the most efficient but hopefully readable
// We need all three args in order to choose the correct register
// reserved registers cannot be used for this argument
// Returns reg but it needs to be dumped later!
Register *get_reg_for_arg(Identifier *arg_id, Identifier *result_id, 
                          Identifier *other_id, bool *reserved)
{
    // if NULL is passed for arg_id, just find an open register
    if (arg_id != NULL)
    {
        // If arg_id is is already in a register, pick it
        Register *cur_reg = get_current_reg(arg_id);
        if (cur_reg != NULL) { return cur_reg; }

        // Check to see if there's an empty register
        for (int i=0; i<NUM_REGISTERS; i++)
        {
            if (REGISTERS[i]->variables_held == NULL)
            {
                return REGISTERS[i];
            }
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
            GList *cur_list = REGISTERS[i]->variables_held;
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
            best_reg = REGISTERS[i];
        }
    }

    return best_reg;
}

/* Returns the index for the given <reg> in the global REGISTERS */
int get_index_for_reg(Register *reg)
{
    for (int i=0; i<NUM_REGISTERS; i++)
    {
        if (reg == REGISTERS[i])
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

/* Ensures that id is in the load_reg, either by loading it or just validating
   that it's already there. If the id has never been used (i.e. not in a register
   nor is it valid on the stack, just clear out the register, and tell the register
   and variable they are in each other (EVEN WHEN UNINITIALIZED!!!).  */
void ensure_register(Identifier *id, Register* load_reg)
{
    // Check to see if it's already loaded
    GList *arg_addrs = id->address_descriptor;
    Register *alt_reg = NULL;
    // Don't allow register loading if we require it to be on the stack
    if (!id->force_on_stack)
    {
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

        // Initial concerns:
        // If this happens then we called ensure register on a register
        // that didn't contain the value, but another did. This seems to indicate
        // something is going wrong with get_reg.

        load_from = repr_reg(alt_reg);
    }
    // If it's not initialized at all, then clearing the register above is enough.
    else if (id->on_stack)
    {
        // Now for the actual loading
        load_from = repr_ident(id);            
        //char *load_from_direct = repr_ident(id);            
        // Want the value, not the address
        //load_from = repr_addr_ind(load_from_direct);
        //free(load_from_direct);
    }

    // Tell the register its holding the variable
    load_reg->variables_held = g_list_prepend(load_reg->variables_held, id);
    // Tell the variable it's also in register
    id->address_descriptor = g_list_prepend(id->address_descriptor, load_reg);

    // If load_from is NULL then the variable was formally unitialized so 
    // there's nowhere to load it from. Dumping, and ensuring they're correct
    // addr details is enough.
    //fprintf(out_file, "ENSURE\n");
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

/* Handle the case of a unary <instr> and write it out to a file */
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
                // If we have to use the stack, don't worry about doing complicated
                // register operations. Just move a const to the stack
                if (assigned->force_on_stack)
                {
                    g_list_free(assigned->address_descriptor);
                    assigned->address_descriptor = NULL;
                    assigned->on_stack = true;

                    char *esp_repr = repr_reg(ESP_REGISTER);
                    char *added_repr= repr_addr_add(esp_repr, assigned->offset);
                    char *result_loc = repr_addr_ind(added_repr);
                    char *const_repr = repr_const(operand->const_val);
                    // And finally do the load
                    write_2instr(MOVE_INSTR, result_loc, const_repr);
                    free(esp_repr);
                    free(added_repr);
                    free(result_loc);
                    free(const_repr);
                }    
                else
                {

                    // Variable's value is now only here.
                    Register *result_reg = get_reg_for_arg(assigned, assigned,
                                                           NULL, reserved);
                    ensure_register(assigned, result_reg);
                    //fprintf(out_file, "BEETWEEN ENSURE AND DUMP\n");
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
                    //fprintf(out_file, "HERE for id %s\n", assigned->symbol);
                    // And finally do the load
                    write_2instr(MOVE_INSTR, result_repr, const_repr);
                    free(result_repr);
                    free(const_repr);
                }
            }
            else if (operand->type == IDENT)
            {
                Identifier *new_id = operand->ident_val;
                // Make sure the OPERAND has a register!
                Register *operand_reg = get_reg_for_arg(new_id, new_id, 
                                                        NULL, reserved);
                ensure_register(new_id, operand_reg);

                // Tell all other registers they no longer have current version
                remove_id_from_regs(assigned);
                // No need to dump as we aren't changing any value
                // Just add the result to this operand's register. Tell the id
                // that it's been assigned here
                operand_reg->variables_held = g_list_prepend(
                                                operand_reg->variables_held,
                                                assigned);

                assigned->on_stack = false;
                g_list_free(assigned->address_descriptor);
                assigned->address_descriptor = NULL;
                assigned->address_descriptor = g_list_prepend(
                                                    assigned->address_descriptor, 
                                                    operand_reg);
                // Note: no direct load instr needed
                // Test whether we need to dump to memory 
                if (assigned->force_on_stack)
                {
                    assigned->on_stack = true;    
                    char *rrepr = repr_reg(operand_reg);
                    move_id_to_stack(rrepr, assigned);
                    free(rrepr);
                }
            }
            else { assert(false); }
            break;
        case PRINT:
            print_variable(instr->arg1->ident_val);
            break;
        default:
            // Can't happen at the moment
            assert(false);
            break;
    }

}

void compile_cond(Instruction *instr)
{
    char *arg1_repr, *arg2_repr;
    // Because we flip order of operands sometimes, keep track
    bool flipped = false;
    // cmp the two args. The first one can't be a constant
    if (instr->arg1->type == CONST && instr->arg2->type != CONST)
    {
        arg1_repr = repr_arg(instr->arg2);
        arg2_repr = repr_arg(instr->arg1);
        flipped = true;
        printf("FLIPPED\n");
    }
    else if (instr->arg1->type == CONST && instr->arg2->type == CONST)
    {
        bool reserved_regs[6] = {0};
        // Get a register for the first constant
        Register *reg = get_reg_for_arg(NULL, NULL, NULL, reserved_regs);
        dump_reg(reg);

        char *reg_repr = repr_reg(reg); 
        char *const_arg_repr = repr_const(instr->arg1->const_val);
        write_2instr_with_option(MOVE_INSTR, DWORD_OPTION, 
                                 reg_repr, const_arg_repr);
        free(const_arg_repr);
        arg1_repr = reg_repr;
        arg2_repr = repr_arg(instr->arg2);
    }
    // Includes case when arg2 == constant but arg1 isn't
    else
    {
        arg1_repr = repr_arg(instr->arg1);
        arg2_repr = repr_arg(instr->arg2);
    }

    write_2instr(REL_INSTR, arg1_repr, arg2_repr);
    free(arg1_repr);
    free(arg2_repr);

    // And then jump appropriately
    eOPCode op_code = instr->op_code;
    if (flipped)
    {
        if (op_code == LT)       { op_code = GT;  }
        else if (op_code == GT)  { op_code = LT;  }
        else if (op_code == LEQ) { op_code = GEQ; }
        else if (op_code == GEQ) { op_code = LEQ; }
    }

    const char *op_code_repr = repr_op_code(op_code);
    write_1instr(op_code_repr, instr->goto_addr->label);
}

void compile_goto(Instruction *instr)
{
    write_1instr(JMP_INSTR, instr->goto_addr->label);
}

void compile_nop(Instruction *instr)
{
    write_0instr(repr_op_code(NOP));
}

void compile_binary(Instruction *instr)
{
    Identifier *result = instr->result;
    // -1->can't reuse result reg, 0->arg1==result, 1->arg2==result
    int reg_to_add = -1;

    assert(instr->arg1 != NULL);
    if (instr->arg1->type == IDENT && 
        instr->arg1->ident_val == result)
    {
        reg_to_add = 0;
    }

    // At the moment ASSIGN is the only unary operator and it's handled above.
    assert(instr->arg2 != NULL);
    // the second arg could be NULL (assignment operator)
    if (reg_to_add != -1
        && instr->arg2->type == IDENT 
        && instr->arg2->ident_val == result)
    {
        reg_to_add = 1;
    }

    bool reserved_regs[6] = {0};
    // Get a register for the result
    Register *result_reg = get_reg_for_arg(result, result, 
                                           NULL, reserved_regs);
    ensure_register(instr->result, result_reg);

    //print_registers(registers, NUM_REGISTERS);
    reserved_regs[get_index_for_reg(result_reg)] = true;

    // Repr of the arg which will opped to the result reg.
    char *op_arg_repr;
    char *result_repr = repr_reg(result_reg);
    switch (reg_to_add)
    {
        case -1:;
            // Load the first arg into the result reg and then op the 
            // other register

            // If it's a constant then we have to load it with the correct 
            // option
            if (instr->arg1->type == CONST)
            {
                // Free up the register so we can throw the constant in
                dump_reg(result_reg);
                char *const_arg = repr_const(instr->arg1->const_val);
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
                ensure_register(instr->arg1->ident_val, result_reg);
            }

            op_arg_repr = repr_arg(instr->arg2);
            break;
        case 0:
            // The first argument is already in result reg just add second
            op_arg_repr = repr_arg(instr->arg2);
            break;
        case 1:
            // The second argument is already in result reg just add first
            op_arg_repr = repr_arg(instr->arg1);
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
    result->address_descriptor = NULL;
    result->address_descriptor = g_list_prepend(result->address_descriptor, result_reg);
    result->on_stack = false;

    write_2instr(repr_op_code(instr->op_code), result_repr,
                 op_arg_repr);

    // And finally check if we need to push it back onto the stack
    if (result->force_on_stack)
    {
        result->on_stack = true;    
        move_id_to_stack(result_repr, result);
    }

    free(result_repr);
    free(op_arg_repr);
}

void perform_instr_actions(Instruction *instr)
{
    GList *instr_acts = instr->actions;
    for (; instr_acts!=NULL; instr_acts=instr_acts->next)
    {
        Action *cur_action = instr_acts->data;
        Identifier *action_id = cur_action->id;
        switch (cur_action->type)
        {
            case FORCE_ID_STACK:
                action_id->force_on_stack = true;
                // Move it to the stack if we can.
                /* For example in case  
                   int x = 0;
                   while (x < 5) { ... }
                   Then the while will force_on_stack but we need to grab x from 
                   its stack location
                */
                if (!action_id->on_stack)
                {
                    Register *cur_reg = get_current_reg(action_id);
                    // if cur_reg==NULL then it's a value initialized in the loop.
                    // We're forcing those on the stack too 
                    //assert(cur_reg!=NULL);
                    if (cur_reg != NULL)
                    {
                        char *rrepr = repr_reg(cur_reg);
                        move_id_to_stack(rrepr, action_id);
                        free(rrepr);
                        action_id->on_stack = true;
                    }
                }
                break;
            case RELEASE_ID_STACK:
                action_id->force_on_stack = false;
                break;
            default:
                assert(false);
                break;
        }
    }
}

/* Compiles an instruction list using a basic register balancing scheme and
    limited out-writing. */
void basic_compile(GPtrArray *instr_list, GHashTable* symbol_table,
                   int num_instrs)
{   
    for (int i=0; i<num_instrs; i++)
    {
        // Check to see if either of the arguments is the result symbol
        Instruction *cur_instr = get_instr(instr_list, num_instrs, i);

        perform_instr_actions(cur_instr);

        // Check whether we need a label
        if (cur_instr->label != NULL)
        {
            write_label(cur_instr);
        }

        // Handle assign seperately
        if (cur_instr->op_code == ASSIGN ||
            cur_instr->op_code == PRINT)
        {
            compile_unary(cur_instr); 
        }
        else if (cur_instr->op_code == GOTO)
        {
            compile_goto(cur_instr);
        } 
        else if (is_relative_op(cur_instr->op_code))
        {
            compile_cond(cur_instr);
        }
        else if (cur_instr->op_code == NOP)
        {
            compile_nop(cur_instr);
        }
        else
        {
            compile_binary(cur_instr);
        }

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
    its proper stack location. OLD and deprecated. */
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
            char *added_repr= repr_addr_add(esp_repr, instr->result->offset);
            char *result_loc = repr_addr_ind(added_repr);
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
                    char *added_repr= repr_addr_add(esp_repr, instr->arg1->ident_val->offset);
                    arg_repr = repr_addr_ind(added_repr);
                    free(esp_repr);
                    free(added_repr);

                    char *intermed_reg_repr = repr_reg(REGISTERS[0]);
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
                arg_repr = repr_addr_ind(arg_repr); 
                free(direct_repr);
            }
            // Save once since we use it 3 times in this block
            char *esp_repr = repr_reg(REGISTERS[0]);
            write_2instr(MOVE_INSTR, esp_repr, arg_repr);
            free(arg_repr);
    
            // Now get the second arguments repr and do the operation
            arg_repr = get_assem_arg_repr(instr->arg2);
            if (instr->arg2->type == IDENT)
            {
                char *direct_repr = arg_repr;
                arg_repr = repr_addr_ind(arg_repr); 
                free(direct_repr);
            }

            write_2instr(repr_op_code(instr->op_code), esp_repr, arg_repr);
            free(arg_repr);
 
            // The final step is calculating where to store the result
            char *add_repr = repr_addr_add(esp_repr, instr->result->offset);
            char *result_loc = repr_addr_ind(add_repr); 
            // Again just using register 0 for now
            write_2instr(MOVE_INSTR, result_loc, esp_repr);
            free(add_repr);
            free(result_loc);
            free(esp_repr);
        }
    }
}

/* Write out some header boilerplate */
void write_header()
{
    fprintf(out_file, "extern printf\n");
    fprintf(out_file, "SECTION .data\n");
    fprintf(out_file, "\t%s\n", NUM_PRINT_FMT);


    fprintf(out_file, "SECTION .text\n");
    fprintf(out_file, "\tglobal _start\n");
    fprintf(out_file, "_start:\n");
}

/* Write out some program terminating boilerplate */
void write_exit()
{
    // Set exit code 0 - OK
    fprintf(out_file, "\tmov ebx, 0\n");
    // Send the exit command to kernel
    fprintf(out_file, "\tmov eax, 1\n");
    // Use the 0x80 interrrupt to call kernel
    fprintf(out_file, "\tint 0x80\n");

}

/* Opens the out_file  */
void init_file(char *fname)
{
    out_file = fopen(fname, "w+");
}

/* Closes the out_file */
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
    basic_compile(instr_list, symbol_table, num_instrs);
    write_exit();
    close_file();
}
