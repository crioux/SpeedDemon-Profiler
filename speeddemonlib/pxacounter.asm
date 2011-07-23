
	; !!! ARMASM.EXE SUCKS !!!

	area	|.text|, code, readonly

	export |arreadpmnc255|
	export |arwritepmnc255|
	export |arreadpmnc27x|
	export |arwritepmnc27x|
	export |arreadccnt255|
	export |arreadccnt27x|

	import |_xyield|	

	export |_xlock|
	export |_xunlock|
	
	export |_xgetstackpointer|


;=========================== arreadpmnc255() =========================
; int arreadpmnc255()
|arreadpmnc255| proc
	mrc p14, 0, r0, c0, c0, 0  
	mov pc, lr
	endp
;......................................................................



;============================ arwritepmnc255() =====================
; void arwritepmnc255(int32 pmnc)
|arwritepmnc255| proc
	mcr p14, 0, r0, c0, c0, 0
	mov pc, lr
	endp
;......................................................................



;========================== arreadpmnc27x() =======================
; int arreadpmnc27x()
|arreadpmnc27x| proc
	mrc p14, 0, r0, c0, c1, 0
	mov pc, lr
	endp
;......................................................................



;========================== arwritepmnc27x() ======================
; void arwritepmnc27x(int32 pmnc)
|arwritepmnc27x| proc
	mcr p14, 0, r0, c0, c1, 0
	mov	pc, lr
	endp
;......................................................................



;========================= arreadccnt255() ==========================
; int arreadccnt255()
|arreadccnt255| proc
	mrc p14, 0, r0, c1, c0, 0
	mov pc, lr
	endp
;......................................................................



;=========================== arreadccnt27x() =========================
; int arreadccnt27x()
|arreadccnt27x| proc
	mrc p14, 0, r0, c1, c1, 0
	mov	pc, lr
	endp
;......................................................................




;=========================== _xlock() =========================
; void _xlock(int *plock)
|_xlock| proc
	stmdb sp!,{sp, lr }
	stmdb sp!,{r0, r4 }
	
|_xlock_wait|
	mov r12,#0
	swp r12,r12,[r0]
	cmp r12,#1
	beq |_xlock_got_it|
	
	mov r4, r0
	bl |_xyield|
	mov r0, r4
	
	b |_xlock_wait|

|_xlock_got_it|
	ldmia sp!,{r0, r4}
	ldmia sp,{sp, pc}
	endp
;......................................................................


;=========================== _xunlock() =========================
; void _xunlock(int *plock)
|_xunlock| proc
	mov r12,#1
	str r12,[r0]
	mov pc, lr
	endp
;......................................................................



;=========================== _xgetstackpointer() =========================
; ADDRESS _xgetstackpointer(void)
|_xgetstackpointer| proc
	mov r0, r13
	mov pc, lr
	endp
;......................................................................



	end
