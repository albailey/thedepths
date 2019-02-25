
.export _showLine;
.export _reset;

;void __fastcall__ showLine();

_showLine:
  ldx #%00011111  ; sprites + background + monochrome 
  stx $2001
  ldy #21  ; add about 23 for each additional line 
  @loop: 
    dey 
    bne @loop 
  dex    ; sprites + background + NO monochrome 
  stx $2001
  rts 

;void __fastcall__ reset();

_reset:
	JMP ($FFFC)
