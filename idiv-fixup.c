#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kdebug.h>
#include <asm/traps.h>
#include <asm/insn.h>
#include <asm/insn-eval.h>

bool fixup_idiv_exception(struct pt_regs *regs) {
    int not_copied, nr_copied;
    unsigned long seg_base = 0;
    unsigned char buf[MAX_INSN_SIZE];
    struct insn insn;

    if (!regs)
        return false;

    if (!user_64bit_mode(regs))
        seg_base = insn_get_seg_base(regs, INAT_SEG_REG_CS);

    if (seg_base == -1L)
        return false;

    not_copied = copy_from_user(buf, (void __user *)(seg_base + regs->ip),
            sizeof(buf));
    nr_copied = sizeof(buf) - not_copied;

    if (nr_copied == 0)
        return false;

    insn_init(&insn, buf, nr_copied, user_64bit_mode(regs));
    insn_get_length(&insn);
    if (nr_copied < insn.length)
        return false;

    // Check if instruction is idiv
    if (insn.opcode.bytes[0] != 0xF7 ||
            X86_MODRM_REG(insn.modrm.value) != 7) {
        return false;
    }

    printk("Skipping idiv, ax=0x%lX, dx=0x%lX\n", regs->ax, regs->dx);

    regs->ip += insn.length;
    return true;
}

static int idiv_die_event_handler(struct notifier_block *self, 
        unsigned long val, void *data) {
    if (val != DIE_TRAP)
        return NOTIFY_DONE;

    struct die_args* args = (struct die_args *)data;
    if (args->trapnr != X86_TRAP_DE)
        return NOTIFY_DONE;

    pr_debug("idiv fixup FIRE!\n");
    if (fixup_idiv_exception(args->regs)) {
        return NOTIFY_STOP;
    }
    return NOTIFY_DONE;
}

static __read_mostly struct notifier_block idiv_die_notifier = {
    .notifier_call = idiv_die_event_handler,
    .next = NULL,
    .priority = 0
};

static int __init idiv_fixup_init(void) {
    int ret;
    ret = register_die_notifier(&idiv_die_notifier);
    if (ret) {
        pr_warn("Failed to register idiv fixup die notifier: %d\n", ret);
    }
    return ret;
}
static void __exit idiv_fixup_exit(void) {
    int ret;
    ret = unregister_die_notifier(&idiv_die_notifier);
    if (ret) {
        pr_warn("Failed to unregister idiv fixup die notifier: %d\n", ret);
    }
}

module_init(idiv_fixup_init);
module_exit(idiv_fixup_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yunhao Tian <t123yh@outlook.com>");
MODULE_DESCRIPTION("idiv fixup");
MODULE_VERSION("0.01");
