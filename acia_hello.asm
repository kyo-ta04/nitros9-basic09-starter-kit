        org     $0000
start   ldx     #msg
loop    lda     ,x+
        beq     done
wait    ldb     $8018   ; Status
        andb    #$02    ; TDRE?
        beq     wait
        sta     $8019   ; Data
        bra     loop
done    swi
msg     fcn     "Hello via ACIA!\n"
        end     start
