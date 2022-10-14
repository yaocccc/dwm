#! /bin/bash
# 获取币价的脚本 不用的话可以去掉

source ~/.profile

this=_coin
s2d_reset="^d^"
color="^c#223344^^b#4E5173^"

main() {
    eth_price=$(curl --connect-timeout 10 -m 20 -s 'https://api.binance.com/api/v3/ticker/price?symbol=ETHUSDT' | jq .price | sed 's/\.*0*"//g')
    text=" ﲹ$eth_price "
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    [ "$eth_price" ] && printf "export %s='%s%s%s'\n" $this "$color" "$text" "$s2d_reset" >> $DWM/statusbar/temp
}

main
