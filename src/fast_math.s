    .section .text

    .global fast_project_star

fast_project_star:
    move.l  d0, d3
    asl.l   #7, d3
    asr.l   #8, d3
    addi.l  #160, d3
    move.w  d3, (a0)

    move.l  d1, d3
    asl.l   #7, d3
    asr.l   #8, d3
    addi.l  #112, d3
    move.w  d3, (a1)

    rts

    .global fast_rotate_point

fast_rotate_point:
    rts