#ifndef _TAG_CONTROL_H
#define _TAG_CONTROL_H

//Define tag control PCR bits
//If bit is set to 1b the function is enabled!

#define RET_TAG_CHECK           0x00000001  //Checks if the return adress tag is set when a ret instuction is performed
#define INV_TAG_CHECK           0x00000002 //Checks if the INVALID flag is not set when a jalr instruction is performed
#define INV_TAG_GEN             0x00000004 //Swtiches on/off the INVALID flag generation if something is read from IO
//#define INV_TAG_GEN             0x00000000 //Swtiches on/off the INVALID flag generation if something is read from IO
#define DEBUG_CHECK             0x00000008 //Traps if a tag other than 0/1/2/3 is in any register (DEBUG purpose)
//#define DEBUG_CHECK             0x00000000 //Traps if a tag other than 0/1/2/3 is in any register (DEBUG purpose)

#define TAG_CTRL_REG_ADDR       0x400

//Wrap the register to be used within asm instuction
#define STR1(x) #x
#define STR(x) STR1(x)
#define ptagctrl                STR(TAG_CTRL_REG_ADDR)


#define write_csr(reg, val) \
  asm volatile ("csrw " reg ", %0" :: "r"(val))

#define read_csr(reg) ({ unsigned long __tmp; \
asm volatile ("csrr %0, " reg : "=r"(__tmp)); \
__tmp; })

#define set_csr(reg, bit) ({ unsigned long __tmp; \
  if (__builtin_constant_p(bit) && (bit) < 32) \
    asm volatile ("csrrs %0, " reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
    asm volatile ("csrrs %0, " reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })

#define clear_csr(reg, bit) ({ unsigned long __tmp; \
  if (__builtin_constant_p(bit) && (bit) < 32) \
    asm volatile ("csrrc %0, " reg ", %1" : "=r"(__tmp) : "i"(bit)); \
  else \
    asm volatile ("csrrc %0, " reg ", %1" : "=r"(__tmp) : "r"(bit)); \
  __tmp; })


 //Only switch on invalid tag generation and leave the rest
#define invalidTagGenOn() ({\
 uint64_t tag_ctrl_state = read_csr(ptagctrl);\
 tag_ctrl_state |=  INV_TAG_GEN;\
 write_csr(ptagctrl,tag_ctrl_state);\
}) \

 //Only switch off invalid tag generation and leave the rest
#define invalidTagGenOff()  ({\
 uint64_t tag_ctrl_state = read_csr(ptagctrl);\
 tag_ctrl_state  &= ~ INV_TAG_GEN;\
 write_csr(ptagctrl,tag_ctrl_state);\
}) \



#endif //_TAG_CONTROL_H
