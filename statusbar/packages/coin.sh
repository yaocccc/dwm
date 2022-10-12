#! /bin/bash
# ICONS 部分特殊的标记图标

source ~/.profile

this=_coin
s2d_reset="^d^"
color="^c#223344^^b#4E5173^"

main() {
    eth_price=$(curl -s 'https://api.binance.com/api/v3/ticker/price?symbol=ETHUSDT' | jq .price | sed 's/\.*0*"//g')

    text=" ﲹ$eth_price "
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    printf "export %s='%s%s%s'\n" $this "$color" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

main
