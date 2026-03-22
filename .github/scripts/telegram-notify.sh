#!/bin/bash

# Скрипт для отправки уведомлений в Telegram

TELEGRAM_BOT_TOKEN="${TELEGRAM_BOT_TOKEN:-7135286706:AAElFGLW2is0wRDSh8CRf04ear2b34EMMns}"
TELEGRAM_USER_ID="${TELEGRAM_USER_ID:-439790276}"
TIME="${TIME:-10}"

if [ "$1" = "success" ]; then
    MESSAGE="SUCCESS ✅"
else
    MESSAGE="FAILED 🚫"
fi

# Получаем информацию о проекте из переменных окружения или используем значения по умолчанию
PROJECT_NAME="${GITHUB_REPOSITORY:-S21_brick_game}"
COMMIT_REF="${GITHUB_REF_NAME:-unknown}"
RUN_ID="${GITHUB_RUN_ID:-unknown}"
RUN_URL="https://github.com/${GITHUB_REPOSITORY}/actions/runs/${GITHUB_RUN_ID}"

TEXT="Deploy status: $MESSAGE %0A%0AProject: $PROJECT_NAME %0AURL: $RUN_URL %0ABranch: $COMMIT_REF"

URL="https://api.telegram.org/bot$TELEGRAM_BOT_TOKEN/sendMessage"

# Отправляем сообщение в Telegram
curl -s --max-time $TIME -d "chat_id=$TELEGRAM_USER_ID&disable_web_page_preview=1&text=$TEXT" $URL > /dev/null

# Проверяем результат отправки
if [ $? -eq 0 ]; then
    echo "Уведомление успешно отправлено в Telegram"
else
    echo "Ошибка при отправке уведомления в Telegram"
    exit 1
fi