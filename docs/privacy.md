# Privacy and local data / Приватность и локальные данные

[← VitaPlane](../README.md)

## English

### Privacy and local data

Data is stored in `ux0:data/vitagram/`. The `db/` directory contains your Telegram
session and account data; `files/` holds downloaded media. Cache retention and size
limits are configurable in Settings → Data and storage. Clean cache now applies the current
retention and size limits; it does not force deletion of all media. Do not share
`db/` or delete it to clear photos.

**Settings → Data protection → Data encryption** protects the database, downloaded media, cache indexes
and settings with a text passphrase (at least 10 characters) or a console-button
sequence (12–32 presses). A longer passphrase offers stronger protection against
guessing. Encryption is bound to the console: copying the card to another Vita
does not unlock the vault. **A forgotten key cannot be recovered.** Resetting the
vault erases local account data and requires signing in again.

<details>
<summary>Encryption scope and limits</summary>

Encryption changes restart the app and convert existing files before Telegram
starts. Interrupted or failed conversions resume on the next launch; until
conversion finishes, some files may remain unencrypted. Auto-lock after inactivity
or sleep hides the screen and pauses playback; background work continues, and
decryption keys remain in RAM.

Completed downloads can briefly exist in plain form before sealing. Incomplete
TDLib downloads in `files/temp/` and `db/temp/` stay unencrypted, including while
paused or after closing the app, so TDLib can resume their saved parts. Recordings,
conversion scratch files, explicit exports and diagnostic logs are outside this
protection. Encryption does not hide filenames or sizes and cannot protect data
from software controlling the running console.

While encryption is on, VitaPlane also limits what the running session leaves in RAM:
freed memory blocks up to 1 MiB are zeroed, the master key is not kept after
unlocking, and session keys are stored masked. Crash dumps (`ux0:data/psp2core-*.psp2dmp`)
contain decrypted memory; VitaPlane's dumps are overwritten and deleted after unlocking
unless a diagnostic log is enabled. Key-entry screens name plugins loaded into VitaPlane
from `tai/config.txt` (`*ALL` or `*VITAGRM01`): they can read the key and messages.
Kernel plugins cannot be detected or stopped by the app.

Without data encryption, console-derived keys protect the database/cache and
obfuscate proxy credentials; OpenPSID is not a secret. Downloaded media remains
unencrypted. SOCKS5 and HTTP proxy credentials travel unencrypted to the proxy;
Telegram message traffic remains MTProto-encrypted.

</details>

Proxy connection diagnostics start automatically before sign-in. Other diagnostic logs are **off by default**. Enable individual logs in
**Settings → About and diagnostics → Diagnostic logs**. Review logs before sharing; report the app version,
steps to reproduce, media type and any on-screen error without including credentials
or account files. The automatic proxy connection log contains only fixed stages, numeric attempt/list indices and error codes; it excludes addresses, secrets, tokens and payloads. It retains at most two 256 KiB files and is deleted on logout. **Settings → About and diagnostics → About** contains project links and privacy notes.

## Русский

### Приватность и локальные данные

Данные находятся в `ux0:data/vitagram/`. Каталог `db/` содержит сессию Telegram
и данные аккаунта, `files/` — загруженные медиа. В подменю «Настройки → Данные и память»
можно задать срок хранения и размер кеша, а также запустить очистку по этим
параметрам: кнопка не удаляет все медиа принудительно. Не публикуйте `db/` и не удаляйте его ради очистки фото.

**Настройки → Защита данных → Шифрование данных** защищает базу, загруженные медиа, индексы кэша
и настройки текстовой фразой (от 10 символов) или последовательностью кнопок
(12–32 нажатия). Длинная фраза устойчивее к перебору. Шифрование привязано
к консоли: перенос карты на другую Vita не разблокирует хранилище.
**Забытый ключ восстановить нельзя.** Сброс хранилища удаляет локальные данные
аккаунта и требует повторного входа.

<details>
<summary>Что защищает шифрование и каковы его ограничения</summary>

Изменение режима шифрования перезапускает приложение и преобразует существующие
файлы до подключения Telegram. Прерванное или неудачное преобразование продолжится
при следующем запуске; до завершения часть файлов может оставаться открытой.
Автоблокировка после бездействия или сна закрывает экран и ставит воспроизведение
на паузу; фоновые операции продолжаются, ключи остаются в оперативной памяти.

Завершённые загрузки кратковременно существуют открытыми до шифрования.
Незавершённые загрузки TDLib в `files/temp/` и `db/temp/` остаются незашифрованными,
в том числе на паузе и после закрытия приложения: это необходимо для докачки
сохранённых частей через TDLib. Записи, временные файлы конвертации, явный экспорт и диагностические журналы не защищены
этим режимом. Шифрование не скрывает имена и размеры файлов и не защищает от
программ, контролирующих работающую консоль.

При включённом шифровании VitaPlane также сокращает следы сеанса в оперативной памяти:
освобождаемые блоки памяти до 1 МиБ обнуляются, мастер-ключ после разблокировки
не хранится, а сеансовые ключи хранятся в маскированном виде. Дампы падений
(`ux0:data/psp2core-*.psp2dmp`) содержат расшифрованную память; дампы VitaPlane
перезаписываются и удаляются после разблокировки, если не включён диагностический журнал.
Экраны ввода ключа показывают плагины, загруженные в VitaPlane из `tai/config.txt`
(`*ALL` или `*VITAGRM01`): они могут прочитать ключ и сообщения. Плагины ядра
приложение обнаружить и остановить не может.

Без шифрования данных ключи на основе идентификатора консоли защищают базу/кэши
и обфусцируют реквизиты прокси; OpenPSID не является секретом. Загруженные медиа
остаются открытыми. Логин и пароль SOCKS5/HTTP передаются прокси без шифрования;
сообщения Telegram остаются зашифрованными MTProto.

</details>

Журнал подключения прокси включается автоматически до входа в аккаунт. Остальные диагностические журналы **выключены по умолчанию**. Отдельные журналы включаются
в **Настройки → О приложении и диагностика → Диагностические логи**. Перед отправкой просмотрите содержимое; в отчёте
укажите версию, шаги воспроизведения, тип медиа и ошибку на экране, исключив
реквизиты и файлы аккаунта. Журнал подключения прокси содержит только фиксированные этапы, числовые номера попыток/элементов списка и коды ошибок без адресов, secret, токенов и содержимого запросов. Хранятся не более двух файлов по 256 КиБ; при выходе из аккаунта они удаляются. **Настройки → О приложении и диагностика → О проекте** содержит ссылки и сведения
о приватности.

WEB proxy support carries Telegram MTProto connections only. VitaPlane uses native
HTTPS with certificate and hostname verification; it does not copy a browser TLS
fingerprint. If system DNS cannot resolve a WEB proxy host, the client queries Google DNS over verified HTTPS; only the hostname is sent, with IPv4 answers cached for at most 60 seconds. Bridge capabilities and session tokens are kept in memory and are
not written to diagnostic logs. Mini Apps, ordinary web pages and update checks
continue using their existing network paths; WEB is not a device-wide VPN.

WEB-прокси передаёт только MTProto-соединения Telegram. VitaPlane использует
нативный HTTPS с проверкой сертификата и имени сервера, не воспроизводя
TLS-отпечаток браузера. Если системный DNS не находит WEB-сервер, клиент обращается к Google DNS по проверенному HTTPS: отправляется только имя сервера, IPv4-ответ хранится не более 60 секунд. Производные ключи доступа и токены сессии остаются в
памяти и не записываются в диагностические журналы. Мини-приложения, обычные
веб-страницы и проверка обновлений продолжают использовать прежние сетевые
маршруты; WEB не является VPN для всей консоли.

The proxy list uses the same console-bound secret protection and optional vault
as the previous single-proxy settings. Availability checks contact each saved
proxy and Telegram; they do not submit credentials to an external scoring service.
Latency results are not saved to disk. Disabling automatic switching also disables
automatic list probes; an explicit check still tests the saved entries.

Список прокси использует прежнюю защиту секретов с привязкой к консоли и
необязательное хранилище. Проверки обращаются к сохранённым прокси и Telegram,
без отправки реквизитов стороннему сервису оценки. Задержки не сохраняются на
диск. Отключение автопереключения также отключает автоматические проверки списка;
ручная проверка по-прежнему доступна.
