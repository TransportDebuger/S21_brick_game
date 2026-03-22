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

# Отправляем сообщение в Telegram с подробным логированием
echo "Отправка уведомления в Telegram..."
echo "URL: $URL"
echo "Text: $TEXT"

response=$(curl -s --max-time $TIME -w " %{http_code}" -d "chat_id=$TELEGRAM_USER_ID&disable_web_page_preview=1&text=$TEXT" $URL)

echo "Raw response: $response"

# Извлекаем HTTP код и тело ответа
http_code=$(echo $response | awk '{print $NF}')
response_body=$(echo $response | sed 's/ [0-9]\{3\}$//')

echo "HTTP Code: $http_code"
echo "Response Body: $response_body"

# Проверяем результат отправки
if [ $http_code -eq 200 ]; then
    echo "✅ Уведомление успешно отправлено в Telegram"
    exit 0
else
    echo "❌ Ошибка при отправке уведомления в Telegram"
    echo "Текст ошибки: $response_body"
    exit 1
fi