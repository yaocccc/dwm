#! /bin/bash
# 获取币价的脚本 不用的话可以去掉

source ~/.profile

this=_coin
s2d_reset="^d^"
color="^c#223344^^b#4E5173^"
signal=$(echo "^s$this^" | sed 's/_//')

update() {
    r=$(curl --connect-timeout 10 -m 20 ccxt.ccxx.icu)
    prices=()
    eth_price=$(echo $r | jq .ETHUSDT)
    [ "$eth_price" ] && prices=(${prices[@]} "ﲹ:$eth_price")
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    if [ "$prices" ]; then
        text=" ${prices[@]} "
        printf "export %s='%s%s%s%s'\n" $this "$color" "$signal" "$text" "$s2d_reset" >> $DWM/statusbar/temp
    fi
}

click() {
    update
    eth_price=$(echo $r | jq .ETHUSDT)
    btc_price=$(echo $r | jq .BTCUSDT)
    notify-send "当前币价" "$(echo -e "\nETH: $eth_price\$\nBTC: $btc_price\$")"
}

case "$1" in
    click) click $2 ;;
    *) update ;;
esac
