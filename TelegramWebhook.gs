const TELEGRAM_TOKEN = '8029735356:AAFkjWeoNbfEsAS5dc8Xz-fdVcIOnsGLDDw';
const CHAT_ID = '-1002757593763';

function doPost(e) {
  const contents = JSON.parse(e.postData.contents);
  const msg = contents.message.text;
  const chat_id = contents.message.chat.id;

  if (msg === '/start') {
    sendMessage(chat_id, "✅ Resuming ESP updates...");
    try {
      const resp = UrlFetchApp.fetch("http://YOUR_ESP_IP/wake");
      Logger.log(resp.getContentText());
    } catch (e) {
      sendMessage(chat_id, "❌ Failed to reach ESP.");
    }
  }
}

function sendMessage(chat_id, text) {
  const url = `https://api.telegram.org/bot${TELEGRAM_TOKEN}/sendMessage`;
  const payload = {
    method: "post",
    contentType: "application/json",
    payload: JSON.stringify({
      chat_id: chat_id,
      text: text
    })
  };
  UrlFetchApp.fetch(url, payload);
}
