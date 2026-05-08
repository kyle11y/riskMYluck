#!/bin/bash

rm -f p1_in p1_out
mkfifo p1_in p1_out

p1=100
p2=100
turn=1

cards=("gray" "green" "red" "black")

draw_cards() {
    local heal=0
    local self_dmg=0
    local enemy_dmg=0

    echo "Drawing 10 cards..."

    for i in {1..10}; do
        card=${cards[$RANDOM % 4]}
        echo "[$i] $card"

        case $card in
            green) ((heal+=5)) ;;
            red) ((self_dmg+=5)) ;;
            black) ((enemy_dmg+=5)) ;;
        esac
    done

    echo "----------------------"
    echo "+$heal HP | -$self_dmg HP | -$enemy_dmg HP"
    echo "----------------------"

    echo "$heal $self_dmg $enemy_dmg"
}

echo "Server started on port 5000..."

nc -l -p 5000 < p1_out > p1_in &

while read msg < p1_in; do
    if [ "$msg" == "DRAW" ]; then

        numbers=($(draw_cards | tee /dev/tty | tail -n 1))

        if [ $turn -eq 1 ]; then
            p1=$((p1 + numbers[0] - numbers[1]))
            p2=$((p2 - numbers[2]))
            turn=2
        else
            p2=$((p2 + numbers[0] - numbers[1]))
            p1=$((p1 - numbers[2]))
            turn=1
        fi

        [ $p1 -gt 100 ] && p1=100
        [ $p2 -gt 100 ] && p2=100
        [ $p1 -lt 0 ] && p1=0
        [ $p2 -lt 0 ] && p2=0

        echo "P1=$p1 | P2=$p2" > p1_out
    fi

    if [ $p1 -le 0 ] || [ $p2 -le 0 ]; then
        echo "Game Over" > p1_out
        break
    fi
done