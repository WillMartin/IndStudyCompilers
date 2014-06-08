#include "register.h"

// Make a copy of the register name so that we can
// free it like everything else 
char *repr_reg(Register *reg)
{
    // All only 3 chars long
    char *repr = malloc(4 * sizeof(char));
    sprintf(repr, "%s", reg->repr);
    return repr;
}

/* Stores a copy of addrs (a identifier's address_descriptor list)
   without the given reg. into new_addrs. Return code is failure status. */
GList *remove_reg_from_addrs(GList *addrs, Register *reg)
{
    GList *head = addrs;
    for (; addrs!=NULL; addrs=addrs->next)
    {
        if (addrs->data == reg)
        {
            // Remove the current link from the chain
            return g_list_delete_link(head, addrs);
        }
    }
    // No exceptions so this will have to due
    // Will happen if reg is not in addrs
    assert(false);
    return NULL;
}

/* Deletes id from the given register */
void remove_id_from_regs(Identifier *id)
{
    GList *addrs = id->address_descriptor;

    // Check all registers it's in
    for (; addrs!=NULL; addrs=addrs->next)
    {
        Register *cur_reg = addrs->data;
        // And then remove it from the register
        GList *vars_held = cur_reg->variables_held;
        GList *vars_head = vars_held;

        for (; vars_held!=NULL; vars_held=vars_held->next)
        {
            Identifier *cur_id = vars_held->data;
            if (cur_id == id)
            {
                // Remove the current link from the chain
                cur_reg->variables_held = g_list_delete_link(vars_head, vars_held);
                break;
            }
        }
        
    }
}

/* Debug method to display <reg> in xml-ish fashion */
void print_register_state(Register *reg)
{
    printf("<REGISTER: %s>\n", reg->repr);
    GList *vars = reg->variables_held;
    printf("\t<VARS>\n");
    for (; vars!=NULL; vars=vars->next)
    {
        printf("\t\t<SYMBOL %s/>\n", ((Identifier*)vars->data)->symbol);
    }
    printf("\t</VARS>\n");
    printf("</REGISTER>\n");
}

/* Prints a list of registers */
void print_registers(Register **regs, int num_regs)
{
    printf("<REGISTER LIST>\n");
    for (int i=0; i<num_regs; i++)
    {
        print_register_state(regs[i]);
    }
    printf("</REGISTER LIST>\n");
}

