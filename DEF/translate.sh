#! /bin/bash
# 利用chatgpt翻译选中的文本
# https://github.com/j178/chatgpt

# 载入环境变量 （apiKey, proxyEndpoint）
source ~/.zshrc
msgid=$((RANDOM%1000))

notify-send -r $msgid "󰊿 AI translator" "."; sleep 0.3
notify-send -r $msgid "󰊿 AI translator" ".."; sleep 0.3
notify-send -r $msgid "󰊿 AI translator" "...";

r=""
chatgpt -p translator -n -d "$(xsel -o)" | while IFS= read -r -n1 char
do
    r="$r$char"
    notify-send -r $msgid "󰊿 AI translator" "$r"
done
