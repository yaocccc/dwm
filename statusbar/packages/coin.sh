#! /bin/bash
# 获取币价的脚本 不用的话可以去掉

source ~/.profile

this=_coin
s2d_reset="^d^"
color="^c#223344^^b#4E5173^"

main() {
    r=$(curl --connect-timeout 10 -m 20 43.158.202.212:7666)
    prices=()
    eth_price=$(echo $r | jq .ETHUSDT)
    apt_price=$(echo $r | jq .APTUSDT)
    axs_price=$(echo $r | jq .AXSUSDT)
    [ "$eth_price" ] && prices=(${prices[@]} "ETH:$eth_price")
    [ "$apt_price" ] && prices=(${prices[@]} "APT:$apt_price")
    [ "$axs_price" ] && prices=(${prices[@]} "AXS:$axs_price")
    sed -i '/^export '$this'=.*$/d' $DWM/statusbar/temp
    if [ "$prices" ]; then
        text=" ${prices[@]} "
        printf "export %s='%s%s%s'\n" $this "$color" "$text" "$s2d_reset" >> $DWM/statusbar/temp
    fi
}

main
