#
# -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# IBM Corporation.
# Portions created by the Initial Developer are Copyright (C) 2004
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

.set r0,0; .set sp,1; .set RTOC,2; .set r3,3; .set r4,4
.set r5,5; .set r6,6; .set r7,7; .set r8,8; .set r9,9
.set r10,10; .set r11,11; .set r12,12; .set r13,13; .set r14,14
.set r15,15; .set r16,16; .set r17,17; .set r18,18; .set r19,19
.set r20,20; .set r21,21; .set r22,22; .set r23,23; .set r24,24
.set r25,25; .set r26,26; .set r27,27; .set r28,28; .set r29,29
.set r30,30; .set r31,31
.set f0,0; .set f1,1; .set f2,2; .set f3,3; .set f4,4
.set f5,5; .set f6,6; .set f7,7; .set f8,8; .set f9,9
.set f10,10; .set f11,11; .set f12,12; .set f13,13; .set f14,14
.set f15,15; .set f16,16; .set f17,17; .set f18,18; .set f19,19
.set f20,20; .set f21,21; .set f22,22; .set f23,23; .set f24,24
.set f25,25; .set f26,26; .set f27,27; .set f28,28; .set f29,29
.set f30,30; .set f31,31
.set BO_IF,12
.set CR0_EQ,2



        .rename H.10.NO_SYMBOL{PR},""
        .rename H.18.XPTC_InvokeByIndex{TC},"XPTC_InvokeByIndex"


# .text section

        .csect  H.10.NO_SYMBOL{PR}
        .globl  .XPTC_InvokeByIndex
        .globl  XPTC_InvokeByIndex{DS}
        .extern .invoke_copy_to_stack
        .extern ._ptrgl{PR}


#
#   XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
#                   PRUint32 paramCount, nsXPTCVariant* params)
#

.XPTC_InvokeByIndex:
		mflr	r0
		stw	r31,-4(sp)
#
# save off the incoming values in the caller's parameter area
#		
		stw	r3,24(sp)		# that
		stw	r4,28(sp)		# methodIndex
		stw	r5,32(sp)		# paramCount
		stw	r6,36(sp)		# params
		stw	r0,8(sp)
		stwu	sp,-136(sp)		#  = 24 for linkage area,  8 * 13 for fprData area, 8 for saved registers

# prepare args for 'invoke_copy_to_stack' call
#		
		lwz	r4,168(sp)		# paramCount
		lwz	r5,172(sp)		# params
		mr	r6,sp			# fprData
		slwi	r3,r4,3			# number of bytes of stack required
								# at most 8*paramCount
		addi	r3,r3,28		# linkage area
		mr	r31,sp			# save original stack top
		subfc	sp,r3,sp		# bump the stack
		addi	r3,sp,28		# parameter pointer excludes linkage area size + 'this'
		
		bl	.invoke_copy_to_stack
		nop
		
		lfd	f1,0(r31)		# Restore floating point registers	
		lfd	f2,8(r31)				
		lfd	f3,16(r31)				
		lfd	f4,24(r31)				
		lfd	f5,32(r31)				
		lfd	f6,40(r31)				
		lfd	f7,48(r31)				
		lfd	f8,56(r31)				
		lfd	f9,64(r31)				
		lfd	f10,72(r31)				
		lfd	f11,80(r31)				
		lfd	f12,88(r31)				
		lfd	f13,96(r31)				
		
		lwz	r3,160(r31)		# that
		lwz	r4,0(r3)		# get vTable from 'that'
		lwz	r5,164(r31)		# methodIndex
		slwi	r5,r5,2			# methodIndex * 4
		lwzx	r11,r5,r4		# get function pointer

		lwz	r4,28(sp)
		lwz	r5,32(sp)
		lwz	r6,36(sp)
		lwz	r7,40(sp)
		lwz	r8,44(sp)
		lwz	r9,48(sp)
		lwz	r10,52(sp)

		bl      ._ptrgl{PR}
		nop		
		
		mr      sp,r31
		lwz	r0,144(sp)
		addi    sp,sp,136
		mtlr    r0
		lwz     r31,-4(sp)
	        blr


# .data section

        .toc                            # 0x00000038
T.18.XPTC_InvokeByIndex:
        .tc     H.18.XPTC_InvokeByIndex{TC},XPTC_InvokeByIndex{DS}

        .csect  XPTC_InvokeByIndex{DS}
        .long   .XPTC_InvokeByIndex     # "\0\0\0\0"
        .long   TOC{TC0}                # "\0\0\0008"
        .long   0x00000000              # "\0\0\0\0"
# End   csect   XPTC_InvokeByIndex{DS}

# .bss section	
