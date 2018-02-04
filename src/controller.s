
    .import     _buttons
    .export     _read_controller_state

.proc _read_controller_state
      lda #$01
      ; While the strobe bit is set, buttons will be continuously reloaded.
      ; This means that reading from JOYPAD1 will only return the state of the
      ; first button: button A.
      sta $4016
      sta _buttons
      lsr a        ; now A is 0
      ; By storing 0 into JOYPAD1, the strobe bit is cleared and the reloading stops.
      ; This allows all 8 buttons (newly reloaded) to be read from JOYPAD1.
      sta $4016
    loop:
      lda $4016
      lsr a	       ; bit0 -> Carry
      rol _buttons  ; Carry -> bit0; bit 7 -> Carry
      bcc loop
      rts
.endproc
