# Features / Возможности

[← VitaPlane](../README.md)

## English

### Screenshots

| Chats | Conversation | Video |
| --- | --- | --- |
| ![Chat list](../media/readme/chats-en.webp) | ![Messages and stickers](../media/readme/conversation-en.webp) | ![Video playback](../media/readme/media-en.webp) |

*English text in these illustrations was translated from the original Russian screenshots.*

### Features

- **Custom theme packs:** install one theme or a whole pack at once: extract/copy
  all theme `.json` files directly into `ux0:data/vitagram/palletes/`, restart,
  and select a theme by filename in Settings. Up to 64 valid files are loaded;
  subfolders, ZIP archives and single colour-catalogue JSON files are not imported. Use [Palette Studio in your browser](https://ing156.github.io/VitaPlane/) to edit
  colors, preview the interface and download a JSON theme. The studio supports all 19 app languages. An
  [offline HTML edition and publishing instructions](../tools/palette-editor/README.md)
  are also available. The web address becomes available after GitHub Pages is enabled.

- **Chats:** private conversations, groups, channels, Telegram folders, archive,
  search, forum topics and channel comments. Chat lists, history and media are cached.
- **Opening chats:** Settings → Chats → When opening a chat selects the latest message, the synchronized Telegram read position, or the last message read on this Vita (default: latest). Read positions load before showing the message. Loading offers cancellation/retry; distant positions show a memory warning, then load a bounded history window with browsing in both directions. Local positions are separate for chats, forum topics and comments. The synchronized mode uses the newer of the Telegram and device read positions, so reopening never moves behind progress already read on this Vita.
- **Bots:** a command menu appears only when commands or a Mini App menu button are provided by the bot. Text reply keyboards, inline callback/URL buttons and callback alerts are supported. Tap buttons directly; Select opens the full button panel, L/R switches its sections, and Left/Right changes pages. Command lists slide up from the bottom; selecting a command or text reply sends a message. Commands, text replies and callback buttons move focus to the latest message; new bot replies remain visible while the user stays at the bottom. A bot menu containing only a Mini App invokes its launch directly and uses the bot-provided title. The panel follows the current palette and font size. The experimental [WebKit viewer](../docs/webkit-vita.md) is a separately downloaded, signed add-on: the base VPK contains no engine. Opening a Mini App checks for a compatible add-on and offers its download size before installation. Ordinary bot buttons need no add-on. Software rendering and frame/IPC buffers stay in RAM, including with storage encryption. Loading the module and Mini Apps still require device testing; remote installation requires the separately published signed add-on release. Buttons requesting personal data, payments or other special actions are not yet supported; they do not silently send text or personal data.
- **Messages:** consecutive messages from the same sender share compact spacing and connected corners regardless of the time gap; the sender header appears once, the tail only on the last bubble, and each message keeps its own timestamp and focus outline. A new day or service message breaks the group. Bold, italic, underline, strikethrough, code/quote highlighting and text spoilers (open the message, then press Cross or tap the hidden line to confirm reveal). Text, replies, forwarding, permission-aware deletion, standard emoji
  reactions (hold Cross for 550 ms on a message; frequent reactions occupy the first row) and voting in polls, including quizzes and multiple-choice polls.
  Links with Telegram preview data show a card with the site, title, description and image.
- **Photos and files:** the Multimedia attachment option selects photos and videos together; albums, attachment batches of up to 10 items, fullscreen
  photos with rotation and 1–5× zoom (touch or right stick up/down), left-stick panning
  while zoomed, and saving files to a download folder. Attachment review offers
  Add text / Edit text; the shared text is sent once as a file or album caption.
- **Stickers and GIFs:** static and animated stickers, recent/favorite/installed
  sticker sets, saved GIFs and GIF search. In the sticker or GIF grid, hold Cross or
  touch an item for 550 ms to preview it. Animations start automatically and loop;
  release to close without sending. A short press sends the item.
  Open a sticker/custom emoji set from the right menu or hold the item in a message.
  In a sticker preview, slide to the Open set button and release. Uninstalled sets
  offer Add set (Triangle or touch).
- **Audio and video:** supported media playback, progressive playback while downloading,
  voice-message recording and video notes from the front or rear camera.
  Hold the Vita in the desired orientation before starting a video note. The
  image inside the circle follows that orientation for the whole take. Double-tap
  the rear touch pad or press Triangle to switch cameras in preview or while recording.
  Voice, audio and video-note playback continues on other screens: a Telegram-style control bar spans the bottom of the conversation pane, reserving space above it for messages and chat actions, and a video note also floats as a small circle you can drag anywhere and tap to pause. Audio controls include play/pause, draggable seeking within the downloaded part, volume/mute and pitch-preserving 0.5–2× speed. Music adds previous/next, shuffle and repeat off/list/track. Tap the speaker for the volume slider; tap its speaker to mute/restore. L+R focuses the player, Left/Right selects a control, Cross activates it, Up/Down adjusts volume/speed or seeks 10 seconds, and Circle returns to the chat. Previous restarts after three seconds, otherwise selects the preceding track. Up to 64 loaded music tracks around the chosen message form a playlist that survives changing chats. Player controls use the active palette; queue controls and speed are disabled for video notes (volume, pause and seeking remain available). Playback settings last for the app session. Music tracks of a chat play as a playlist: the next track starts when one ends and keeps downloading in the background. Music continues when another chat is opened or another voice message is selected; only pressing play on another message replaces it. Music and video with sound (clips and video notes) share one audio output: starting one pauses the other.
  Fullscreen video: Left/Right selects 10-second seek steps; holding repeats faster
  after two and four seconds. The time and cursor update while held, and releasing
  applies the target and restores the prior play/pause state. Tap or drag the
  progress track to seek, including while the controls are hidden.
- **User colors:** Settings → Interface → Use user colors in the interface (enabled by default). Telegram colors are used for sender names, empty avatars, forwards, link previews and reply headers; replies use the original author’s color.
- **Split keyboard:** spread two fingers horizontally across the built-in keyboard to move the keys towards the edges; pinch them inward to restore the full layout. Settings → Controls and input → Keyboard → Keyboard spread selects Small (the original spacing), Medium (default), or Wide. The choice survives restarts. Theme colors and the wide spacebar are preserved.
- **Display mode:** Settings → Interface → Display mode switches between Classic (default) and Combined. Combined keeps a compact, single-line chat list on the left, history on the right and folders hidden behind a 12 px strip at the left edge. Swipe the strip right or tap it to reveal folders with a 180 ms slide that pushes only the content below the fixed top header without shrinking text. With folders folded, all screens have zero translation; only the chat list reserves space for the edge strip, keeping the message pane and right edge in their fixed bounds. Swipe left, tap outside or use the return control to close; tap a folder to select it and close. Long folder lists scroll vertically. From the focused chat list, Left opens folders; Up/Down selects, Cross confirms and Right/Circle closes. With folders hidden, the top header shows the active folder name and updates when switching with L/R. Classic retains its top folder tabs. Search, attachment selection, stickers, GIFs, reactions and recording open in separate windows over the Combined layout, keeping the background chat geometry and text size. Tool windows have a return button and own input; chat actions beneath them are disabled. A touch drag scrolls the pane where it began, independently of controller focus, and keeps that pane until release. Left or Circle focuses the chat list; Right returns to history; Cross opens the selected chat. Start opens settings while the chat list has focus. Touch a chat to switch directly. Chat loading/retry, forum topics, context menus and individual posts stay inside the message pane. Post text wraps to its width without scaling the font; media controls and touch targets follow the same geometry. Explicit fullscreen photo and video viewers use the whole display. Combined mode hides and disables the bottom-left attachment corner across chat tools; hold Square to choose an attachment. The choice survives restarts.
- **Interface:** native Vita keyboard or the localized in-app keyboard
  (Settings → Controls and input → Keyboard → Keyboard type), Cyrillic and color emoji, Light/Dark/VHBB themes, font size
  and interface language settings. Hold D-pad up/down or the left stick to scroll
  lists with acceleration. Settings → Controls and input → Scroll speed selects Normal,
  Fast or Very fast: up to 1, 2 or 3 pages per 0.5 seconds. Normal is the default.
  Dark is the initial theme. In a chat, tap Square to compose text; hold it for
  250 ms to open the radial menu. The wheel slides in and unfolds over 350 ms.
  The left stick selects Multimedia (up), GIFs (upper right), File (lower right),
  Voice (lower left) or Stickers (upper left); neutral selects text. While holding,
  either L or R toggles between a new message and a reply to the captured message.
  Both controls stay visible in the center. The 496 px wheel uses rounded glass
  petals, blue/theme-accent glow, icons and detail captions, with a 216 px center.
  Release Square to confirm after a 180 ms closing animation, or Circle to cancel.
  Reply context follows text, stickers, GIFs, voice and attachment batches until
  sending or cancellation. The in-app message window supports line breaks and
  sending with Send or Start; the system keyboard remains the default.
  Hold a key for 300 ms to open alternatives, slide to a choice and release to insert.
  Square deletes before the cursor, Triangle inserts a space, and L/R move the cursor.
  Hold Square, the on-screen Delete key, or Cross on Delete to repeat deletion
  after 350 ms, then every 60 ms. Releasing or leaving the key stops it.
  With buttons, hold Cross and select with Left/Right; Circle cancels the choice.
  Russian е offers ё and 4, ь offers ъ (also in uppercase); numbers 1–0 align to ц–х.
  Comma/period beside the spacebar offer punctuation choices; ?123 shows all symbols.
  Shift applies to one entered character; double-tap within 350 ms locks Caps Lock,
  and another tap unlocks it. Messages start with a capital and capitalize after
  sentence-ending punctuation plus whitespace or a newline. Search, credentials
  and other exact-input fields do not use automatic capitalization.
  Settings → Controls and input → Keyboard → Input languages selects the enabled languages. The default
  is English plus the supported console language, or English alone. The language
  key cycles enabled languages on a short press. Hold it for 300 ms to choose
  from a popup carousel, slide or use Left/Right while holding Cross, then release.
  Large sets scroll at the popup edges; Circle cancels selection.
  The 15 Latin/Cyrillic profiles use language-specific
  layouts, accents and case rules. Japanese, Korean and both Chinese variants use
  the system IME for composition, preserving the current draft. The initial profile
  follows the console language when enabled; an explicit choice is remembered until
  the app closes. Input-language preferences survive restarts independently of the UI language.
  Vault passphrases always use the in-app keyboard; controller hints are placed
  directly on its keys. See [keyboard layouts](../docs/keyboard-layouts.md).
- **Connection and privacy:** MTProxy, SOCKS5, HTTP CONNECT and WEB (HTTPS carrier) proxies, proxy-link
  and QR import, optional data encryption and automatic screen locking.
- **Updates:** signed VPK packages from GitHub Releases, with manual and automatic
  update checks.

### Custom fonts

Copy `.ttf` or `.otf` files directly into `ux0:data/vitagram/fonts/` (up to 8 MiB per file,
64 entries), then choose **Settings → Interface → Font**. The folder is scanned at startup and each
opening of the picker; no restart is needed to discover newly copied files. Only the selected
font is loaded, in the background when selected. The filename is saved for the next launch.
Built-in fonts supply missing characters; emoji and controller icons keep their own renderer.
Choose **Default** to restore the built-in fonts. Unreadable, corrupt, bitmap-only or
non-Unicode fonts cannot replace the current font. A missing or invalid saved font falls
back to the default at startup. Font files remain ordinary, unencrypted user resources.

### Experimental audio calls

In a call-enabled build, open a private chat and select **Start → Audio call**.
The call window shows the contact's avatar and name. Select **Call** to dial
or **Cancel** to return to the chat. Incoming audio calls open their own window
while VitaGram is open, with **Accept** and **Decline**. During a call you can
mute the microphone or hang up. Incoming and outgoing calls play different
sounds while waiting for an answer. A call card in a private chat shows the result,
time and duration where available. Tap an audio call card or select it with
**Cross** to open a new call window.

### Limitations

- Media support is bounded by codecs, resolution and file size. The player supports
  H.264, VP8 and VP9; H.265/AV1 decoders are not included. The playback menu → Quality selects a compatible Telegram variant for the current video.
  Settings → Data and storage → Maximum video quality limits the choices to Minimum (default), up to 360p,
  480p or 720p. Switching quality keeps the position and play/pause state; fallback video limits are
  720p by pixel area and 1 GiB. Video is read incrementally; the size limit reserves headroom for Vita’s file offsets, including encrypted-file overhead.
- Progressive audio/video can start after a readable 256 KiB prefix, but some MP4
  files need a complete download. While a video streams, seeking is limited to the
  already downloaded part; portrait-video preparation requires the completed file,
  and prepared portrait copies have additional seek restrictions.
- Calls, secret chats, group administration, paid/custom reactions, creating polls
  or editing forum topics, and searching for sticker sets are not implemented.
- Presence follows Telegram privacy settings. Sticker-set and topic lists still
  need Telegram responses; cached previews do not provide fully offline menus.

## Русский

### Скриншоты

| Чаты | Переписка | Видео |
| --- | --- | --- |
| ![Список чатов](../media/readme/chats.webp) | ![Сообщения и стикеры](../media/readme/conversation.webp) | ![Воспроизведение видео](../media/readme/media.webp) |

### Возможности

- **Свои темы и наборы:** можно установить одну тему или сразу набор:
  распакуйте/скопируйте все файлы тем `.json` прямо в `ux0:data/vitagram/palletes/`,
  перезапустите приложение и выберите тему по имени файла в настройках.
  Загружаются до 64 корректных файлов; вложенные папки, ZIP и единый JSON-каталог
  цветов не импортируются. [Откройте мастер дизайна в браузере](https://ing156.github.io/VitaPlane/),
  настройте цвета, проверьте предпросмотр и скачайте JSON темы. Мастер поддерживает
  все 19 языков приложения. Доступны также
  [автономный HTML и инструкция публикации](../tools/palette-editor/README.md).
  Веб-адрес заработает после включения GitHub Pages.

- **Чаты:** личная переписка, группы, каналы, папки Telegram, архив, поиск,
  темы форумных групп и комментарии каналов. Списки чатов, история и медиа кэшируются.
- **Открытие чатов:** Настройки → Чаты → При открытии окна чата: последнее сообщение, общая отметка прочитанного Telegram или последнее прочитанное на этой Vita (по умолчанию — последнее сообщение). Чат открывается после загрузки нужной позиции; во время ожидания доступна отмена, при ошибке — повтор. Для далёкой позиции показывается предупреждение о памяти, после закрытия которого загружается ограниченный участок истории с перелистыванием в обе стороны. Локальные отметки хранятся отдельно для чатов, тем форума и комментариев. В режиме общей отметки Telegram выбирается более новая из отметок Telegram и устройства, чтобы при повторном открытии не возвращаться за уже прочитанное на Vita.
- **Боты:** меню появляется только при наличии команд или кнопки мини-приложения от бота. Поддерживаются текстовые ответные клавиатуры, inline callback/URL-кнопки и уведомления в ответ на нажатие. Кнопки доступны касанием; Select открывает полную панель, L/R переключает разделы, влево/вправо — страницы. Список команд выезжает снизу; выбор команды или текстовой ответной кнопки отправляет сообщение. После команды, текстовой или callback-кнопки фокус переходит к последнему сообщению; новые ответы бота остаются видны, пока пользователь находится внизу чата. Единственная кнопка мини-приложения вызывает запуск сразу и использует название от бота. Панель следует текущей палитре и размеру шрифта. Экспериментальный [WebKit](../docs/webkit-vita.md) вынесен в отдельно скачиваемое подписанное дополнение: основной VPK не содержит движок. При открытии мини-приложения клиент проверяет совместимость и предлагает скачать дополнение с указанием размера. Обычные кнопки ботов обходятся без него. Программная отрисовка и кадры/IPC остаются в RAM, в том числе при шифровании. Загрузка модуля и Mini Apps ещё требуют проверки на Vita; для сетевой установки нужен отдельно опубликованный подписанный выпуск дополнения. Запросы персональных данных, платежи и другие специальные действия пока не поддерживаются; такие кнопки не отправляют вместо этого текст или персональные данные.
- **Сообщения:** жирный текст, курсив, подчёркивание, зачёркивание, выделение кода/цитат и текстовые спойлеры (откройте сообщение и нажмите × либо коснитесь скрытой строки для подтверждения). Текст, ответы, пересылка, удаление с проверкой прав, обычные
  emoji-реакции (удерживайте × на сообщении 550 мс; частые реакции — в первой полосе) и голосование в опросах, включая викторины и множественный выбор.
  Ссылки с данными предпросмотра от Telegram показывают карточку сайта с заголовком, описанием и изображением.
- **Фото и файлы:** единый пункт «Мультимедиа» для выбора фото и видео; альбомы, отправка до 10 вложений за раз, полноэкранные фото
  с поворотом и масштабом 100–500% (жестом или правым стиком вверх/вниз), перемещением
  левым стиком при увеличении, сохранение файлов в папку загрузок. Перед отправкой
  вложений пункт «Добавить текст» / «Изменить текст» открывает редактор; общий текст
  отправляется один раз как подпись к файлу или альбому.
- **Стикеры и GIF:** статичные и анимированные стикеры, недавние, избранные
  и установленные наборы, сохранённые GIF и поиск анимаций. В сетке стикеров или GIF
  удерживайте × или касание элемента 550 мс для предпросмотра. Анимация запускается
  автоматически и повторяется; отпускание закрывает окно без отправки.
  Короткое нажатие отправляет выбранный элемент. Набор стикера или пользовательского
  эмодзи открывается из правого меню или удержанием в сообщении. В предпросмотре
  стикера сдвиньте палец на кнопку открытия набора и отпустите. Неустановленный
  набор можно добавить кнопкой △ или касанием.
- **Аудио и видео:** воспроизведение поддерживаемых медиа, просмотр во время
  загрузки, запись голоса и видеокружков с фронтальной или задней камеры.
  Голос, аудио и видеокружки продолжают играть на других экранах: внизу области переписки появляется панель в стиле Telegram; сообщения и кнопки чата располагаются выше неё, а кружок ещё и отображается маленьким кругом, который можно перетащить в любое место и коснуться для паузы. Аудиопанель предлагает пуск/паузу, перемотку касанием или перетаскиванием в пределах загруженной части, громкость с отключением звука и скорость 0,5–2× без изменения высоты голоса. Для музыки доступны предыдущий/следующий трек, перемешивание и повтор: выключен / список / трек. Касание динамика открывает ползунок громкости; динамик в открытой панели отключает и восстанавливает звук. L+R переводит фокус на плеер; влево/вправо выбирает действие, крестик выполняет его, вверх/вниз меняет громкость/скорость либо перематывает на 10 секунд; круг возвращает управление чату. Предыдущий трек после трёх секунд сначала возвращает в начало. Очередь содержит до 64 загруженных в историю музыкальных сообщений вокруг выбранного и сохраняется при смене чата. Палитра плеера следует теме. У видеокружков недоступны музыкальная очередь и скорость; громкость, пауза и перемотка сохраняются. Настройки воспроизведения действуют до закрытия приложения. Музыка чата играет как плейлист: следующий трек стартует после окончания предыдущего и догружается в фоне. Музыка не прерывается при переходе в другой чат или выборе другого голосового сообщения; её заменяет только запуск другого. Музыка и видео со звуком (ролики и кружки) делят один звуковой выход: запуск одного ставит на паузу другое.
- **Цвета пользователей:** переключатель «Учитывать цвета пользователей при построении интерфейса» расположен в «Настройки → Интерфейс» сразу под темой и включён по умолчанию. Цвета Telegram используются для имён, аватаров без фото, пересылок, предпросмотра ссылок и цитат ответов. Цитата использует цвет автора исходного сообщения.
- **Раздельная клавиатура:** разведите два пальца по горизонтали на встроенной клавиатуре, чтобы сдвинуть клавиши к краям. Сведение возвращает обычную раскладку. В «Настройки → Управление и ввод → Клавиатура → Раздвигание клавиатуры» доступны небольшое (прежняя геометрия), среднее (по умолчанию) и широкое раздвигание; выбор сохраняется после перезапуска. Цвета темы и широкий пробел сохраняются.
- **Интерфейс:** системная клавиатура Vita или локализованная встроенная клавиатура
  («Настройки → Управление и ввод → Клавиатура»), кириллица и цветные emoji, темы Светлая/Тёмная/VHBB,
  настройка размера шрифта и языка. Удержание стрелок вверх/вниз или левого стика прокручивает списки с ускорением. «Настройки → Управление и ввод → Скорость прокрутки»:
  «Норма», «Быстрая» и «Очень быстрая» — до 1, 2 и 3 страниц за 0,5 секунды.
  По умолчанию выбрана «Норма», тема тёмная. В чате короткое □ открывает текст;
  удержание 250 мс — радиальное меню с выездом и раскрытием за 350 мс.
  Левый стик выбирает «Мультимедиа» сверху, GIF справа сверху, «Файл» справа снизу,
  «Голосовое» слева снизу или «Стикеры» слева сверху; нейтральное положение — текст.
  При удержании любая из кнопок L/R переключает «Новое ↔ Ответ» на зафиксированное
  сообщение; обе подсказки постоянно видны в центре. Колесо диаметром 496 px
  использует скруглённые стеклянные лепестки, свечение цвета акцента темы,
  иконки и вторые строки подписей; диаметр центральной области — 216 px.
  Отпустите □ для подтверждения и закрытия за 180 мс; ○ отменяет выбор.
  Контекст ответа сохраняется для текста, стикеров, GIF, голоса и вложений до
  отправки или отмены. Встроенное окно сообщения поддерживает
  переносы строк и отправку кнопкой «Отправить» или Start.
  По умолчанию клавиатура системная. Удержание клавиши 300 мс открывает варианты:
  сдвиньте палец к нужному и отпустите. □ удаляет символ перед курсором,
  △ вставляет пробел, L/R перемещают курсор. Удержание □, экранной клавиши
  удаления или × на ней повторяет удаление через 350 мс, затем каждые 60 мс.
  Отпускание или уход с клавиши прекращает удаление. На кнопках — удерживайте × и выбирайте
  стрелками влево/вправо; ○ отменяет выбор. У «е» доступны «ё» и «4», у «ь» — «ъ»,
  в том числе в верхнем регистре. Цифры 1–0 расположены на «ц»–«х».
  Запятая и точка рядом с пробелом также предлагают варианты; все символы — в ?123.
  Shift действует на один введённый символ; двойное нажатие за 350 мс включает
  Caps Lock, следующее отключает. В сообщениях заглавная буква ставится в начале,
  после конца предложения с пробелом и после переноса строки. Поиск, пароли и
  остальные поля точного ввода используют ручной регистр.
  В «Настройки → Управление и ввод → Клавиатура» выбираются тип клавиатуры и языки ввода. По умолчанию
  включены английский и поддерживаемый язык системы, иначе — только английский.
  Короткое нажатие клавиши языка переключает на следующий включённый язык.
  Удержание 300 мс открывает карусель: сдвиньте палец или выбирайте стрелками
  влево/вправо при зажатой ×, затем отпустите; ○ отменяет выбор. Длинный список
  прокручивается у краёв панели. Для 15 языков на латинице и кириллице доступны
  свои раскладки, диакритика и правила регистра. Японский, корейский и оба китайских
  варианта используют системный ввод с сохранением набранного текста. Начальная
  раскладка соответствует языку системы, если он включён; ручной выбор запоминается
  до закрытия приложения. Список языков сохраняется между запусками независимо от языка интерфейса.
  Пароли хранилища всегда вводятся встроенной клавиатурой; подсказки управления
  расположены прямо на её клавишах. [Список раскладок](../docs/keyboard-layouts.md).
- **Подключение и приватность:** MTProxy, SOCKS5, HTTP CONNECT и WEB (транспорт HTTPS), импорт прокси
  по ссылке и QR-коду, необязательное шифрование данных и автоблокировка экрана.
- **Обновления:** подписанные VPK из GitHub Releases, ручная и автоматическая проверка.

### Собственные шрифты

Скопируйте `.ttf` или `.otf` непосредственно в `ux0:data/vitagram/fonts/` (до 8 МиБ на
файл, до 64 вариантов), затем выберите **Настройки → Интерфейс → Шрифт**. Папка сканируется при
запуске и каждом открытии списка: новые файлы доступны без перезапуска. При выборе
шрифт загружается в фоне; имя выбранного файла сохраняется для следующего запуска.
Недостающие символы берутся из встроенных шрифтов; emoji и значки кнопок сохраняют
свою отрисовку. Вариант **Стандартный** возвращает встроенные шрифты. Ошибочный,
нечитаемый, растровый или не поддерживающий Unicode файл не заменяет рабочий шрифт.
Если сохранённый файл удалён или повреждён, при запуске используется стандартный.
Файлы шрифтов остаются обычными пользовательскими ресурсами без шифрования.

### Экспериментальные аудиозвонки

В сборке с поддержкой звонков откройте личный чат и выберите **Start → Аудиозвонок**.
В окне звонка появятся аватар и имя собеседника. Выберите **Позвонить**, чтобы
начать вызов, или **Отмена**, чтобы вернуться в чат. Входящий аудиозвонок
показывается в открытом VitaGram с кнопками **Принять** и **Отклонить**.
В ожидании ответа входящий и исходящий вызовы звучат по-разному.
Во время разговора можно выключить микрофон или дать отбой. В карточке звонка
в личном чате видны результат, время и длительность разговора. Коснитесь карточки
аудиозвонка или выберите её кнопкой **×**, чтобы открыть окно нового вызова.

В **Настройки → Подключение → Звук звонков** можно выбрать **Низкая задержка**.
В этом режиме Vita использует меньший буфер входящего звука, поэтому при
нестабильной связи голос может чаще прерываться. По умолчанию выбран
**Стабильный** режим. Выбор сохраняется и применяется со следующего звонка.

### Ограничения

- Поддержка медиа зависит от кодека, разрешения и размера. Есть H.264, VP8 и VP9;
  декодеры H.265/AV1 не включены. В меню проигрывания → Качество выбирается вариант текущего видео.
  Настройки → Данные и память → Максимальное качество видео ограничивает выбор: минимальное (по умолчанию),
  до 360p, 480p или 720p. Смена качества сохраняет позицию и состояние воспроизведения; при его отсутствии действуют лимиты исходника: 720p по площади
  кадра и 1 ГиБ. Видео читается по частям; предел размера оставляет запас для файловых смещений Vita, включая накладные расходы шифрования.
- Просмотр видео/аудио может начаться после непрерывных 256 КиБ от начала файла,
  но некоторым MP4 нужна полная загрузка. Во время потокового просмотра перемотка
  доступна в пределах уже загруженной части; подготовка вертикального видео требует
  докачки, у подготовленных вертикальных копий есть дополнительные ограничения перемотки.
- В полноэкранном видео влево/вправо выбирает позицию с шагом 10 секунд. Удержание
  ускоряет повтор через 2 и 4 секунды. Отметка и время обновляются при удержании,
  переход выполняется после отпускания с сохранением прежнего режима воспроизведения.
  Касание или перетаскивание по шкале тоже перематывает, даже если панель скрыта.
- Не реализованы звонки, секретные чаты, управление группами, платные/пользовательские
  реакции, создание опросов, редактирование тем форума и поиск наборов стикеров.
- Статусы присутствия зависят от приватности Telegram. Списки наборов стикеров
  и тем требуют ответа сервера; кэш миниатюр не делает эти меню полностью офлайн.

### Settings navigation / Навигация по настройкам

Settings now has eight sections: Interface, Chats, Controls and input, Data and
storage, Connection, Data protection, Updates and installation, and About and
diagnostics. Log out is a separate final row. Cache limits and cleanup are directly
in Data and storage. Circle or the on-screen Back control returns to the previous
menu and restores its selection and scroll position.

В настройках восемь разделов: «Интерфейс», «Чаты», «Управление и ввод»,
«Данные и память», «Подключение», «Защита данных», «Обновления и установка»,
«О приложении и диагностика». Выход из аккаунта — отдельная последняя строка.
Лимиты и очистка кэша находятся прямо в «Данные и память». Круг или экранная
кнопка «Назад» возвращает в предыдущее меню с восстановлением выбора и прокрутки.

### Переход к дате / Jump to date

Дата первого видимого сообщения закреплена над перепиской. Нажмите на неё или на разделитель дней, чтобы открыть календарь. L/R меняют месяц, стрелки выбирают день, {cross} переходит к последнему доступному сообщению не позднее выбранного дня, {circle} закрывает календарь. Можно нажать день на экране. Будущие даты недоступны; начало календаря — август 2013 года. Загружается небольшое окно истории, а не вся переписка. Доступ к удалённым сообщениям и скрытой истории определяется Telegram. В темах и комментариях поиск ограничен текущим обсуждением.

Tap the floating date or a day separator to open the themed calendar. L/R change months, the D-pad selects a day, Cross jumps and Circle returns. Future days are disabled; the calendar starts in August 2013. A bounded history window opens at the last available message no later than the selected day; Telegram controls history availability. Topics and comments keep their current scope.

### Downloads and background playback / Загрузки и фоновое воспроизведение

Open the bottom Downloads panel to manage up to eight documents. Tap a file to
pause or resume it, use −/+ to change its priority (1 is lowest, 3 is highest), or tap Cancel to remove
the task and its partial file. With the controller, L+R focuses the panel, Cross
opens the list, Up/Down selects a file, Left/Right changes priority, Square
cancels and Circle returns. When music is also active, swipe vertically on the
panel title or its shaded edge to switch panels.

New Telegram document downloads write to a temporary file in the chosen folder.
Once TDLib confirms completion, final renames run in completion order ahead of other storage jobs. A running cache copy yields between blocks. The account journal records
the destination, confirmed prefix, priority and pause state. After restart,
VitaPlane verifies the source message before reusing the partial file.
Already cached documents can also be copied to a chosen folder.

Откройте нижнюю панель «Загрузки», чтобы управлять очередью из восьми документов.
Касание строки ставит загрузку на паузу или продолжает её. Кнопки −/+ меняют
приоритет: 1 — низкий, 2 — обычный, 3 — высокий. «Отмена» удаляет задание и его недокачанный файл.
На контроллере L+R переводит фокус на панель, крестик открывает список,
вверх/вниз выбирают файл, влево/вправо меняют приоритет, квадрат отменяет
загрузку, круг возвращает в чат. Если играет музыка, вертикальный свайп
по заголовку или затенённому краю переключает панели.

Новый документ скачивается во временный файл в выбранной папке и получает
окончательное имя после подтверждения от TDLib. Готовые файлы переименовываются в порядке завершения перед остальными заданиями очереди загрузок. Копирование из кеша уступает им очередь между блоками. Журнал аккаунта хранит папку, подтверждённую
позицию, приоритет и состояние паузы. После перезапуска VitaPlane проверяет
исходное сообщение и продолжает загрузку с сохранённой позиции.
Уже скачанный документ можно скопировать из кеша в выбранную папку.

При зависании на 100% включите журнал производительности до начала загрузки.
В `ux0:data/vitagram/performance.log` строки `document` связаны номером
`attempt` в пределах сеанса. `bytes_full` означает получение всех байтов по
счётчику, `td_complete` — подтверждение TDLib. За постановкой в очередь
(`rename_queued`) и началом переименования (`rename_started`) следует
`rename_ok` либо `rename_error` с системным кодом `errno`. После обработки
результата `row_removed` отмечает удаление задания из списка. При ротации
предыдущие строки доступны в `performance.previous.log`.

Enable **Settings → Data and storage → Ask where to save downloads** to choose a
folder for each document before downloading. The folder browser supports drives,
subfolders and creating a folder. Square confirms the destination; Circle returns
to the parent, then closes from the drive list. Cancelling starts no download and
does not change the default download folder. The last confirmed destination is
remembered across restarts. If it is unavailable, the picker opens the configured
default folder. The list shows folders first and also shows files; files are
informational, while Square saves into the current folder.

Включите **Настройки → Данные и память → Спрашивать, куда сохранить**, чтобы перед
скачиванием документа выбирать папку. В выборе папок доступны накопители,
подпапки и создание папки. Square подтверждает папку; Circle поднимается на уровень
выше и закрывает окно из списка накопителей. Отмена не запускает скачивание и не
меняет папку по умолчанию. Последняя подтверждённая папка запоминается между
запусками. Если она недоступна, открывается папка по умолчанию из настроек.
Список показывает сначала подпапки, затем файлы. Файлы показаны для ориентира;
Square сохраняет в текущую папку.

Voice messages, music, video notes and videos keep playing while browsing chats.
The bottom panel provides playback, seeking and volume controls. Tap the speed
button to open a separate choice panel (0.5×–2×). L+R focuses the player; select
speed with the D-pad, confirm with Cross and close with Circle. Video speed
changes require a seekable source. In an open channel post, including a poll,
SELECT opens the attachment menu containing Reactions.

Голосовые, музыка, видеокружки и видео продолжают воспроизведение при просмотре
чатов. Нижняя панель позволяет управлять воспроизведением, перемоткой и громкостью.
Кнопка скорости открывает отдельную панель выбора (0,5×–2×). L+R переводит фокус
на плеер; крестовина выбирает скорость, Cross подтверждает, Circle закрывает.
Изменение скорости видео доступно для источников с перемоткой. В открытом посте
канала, включая опрос, SELECT открывает меню вложений с пунктом «Реакции».

In the custom keyboard, tap the visible draft text to place the insertion point.
Hold for 350 ms to open a themed magnifier, then slide your finger to move the
cursor. Hold at the top or bottom edge to scroll through long drafts. Lifting the
finger closes the magnifier; controller input cancels the gesture. Password fields
never show entered text in a magnifier. L/R still move the cursor by character.

В кастомной клавиатуре касание видимого текста устанавливает курсор. Удерживайте
палец 350 мс, чтобы открыть лупу в цветах темы, затем ведите его по тексту для
перемещения курсора. У верхнего или нижнего края прокручивается длинный черновик.
Лупа закрывается при отпускании пальца; нажатие кнопки отменяет жест. Поля пароля
не показывают введённый текст в лупе. L/R по-прежнему перемещают курсор по символам.

### Editing your text / Редактирование своего текста

Select your sent text message or open its post, then choose **Edit text** in the
right-hand menu (Start). The keyboard opens with the existing text; **Save** or
Start applies the change, and Circle cancels. This uses Telegram's
[editMessageText](https://core.telegram.org/tdlib/docs/classtd_1_1td__api_1_1edit_message_text.html)
and `messageProperties.can_be_edited`, so Telegram determines current rights and
editing deadlines. No fixed editing time window is assumed by the client.

The supported subset is ordinary outgoing text up to **1023 UTF-8 bytes**, with
automatically detected links, mentions and similar entities. This keeps the whole
original inside the existing keyboard. Media captions, albums, polls, rich-message
blocks, forwarded/bot messages and manual formatting/custom emoji are excluded;
the client has no entity editor and must not strip them. Link preview options are
preserved. Sending/failed messages cannot be edited. Errors and timeouts retain
the typed draft for retry; changes appear after Telegram confirms them. If the
original changes elsewhere, cancel and reopen editing to avoid overwriting it.
Leaving the chat/topic or starting another composition cancels the local editor;
a request already sent to Telegram may still complete on the server.

Выберите своё отправленное текстовое сообщение или откройте пост и нажмите
**Редактировать текст** в правом меню (Start). В клавиатуре появится исходный текст;
**Сохранить** или Start применяет правки, Circle отменяет. Права и допустимый срок
редактирования определяет Telegram через TDLib; фиксированный срок клиент не задаёт.

Поддерживается обычный исходящий текст до **1023 байт UTF-8**, включая автоматически
распознаваемые ссылки и упоминания. Исходный текст должен целиком помещаться в
текущую клавиатуру. Подписи медиа, альбомы, опросы, блоки rich message, пересылки,
сообщения через ботов, ручное форматирование и пользовательские эмодзи исключены:
редактора сущностей в клиенте нет, поэтому оформление не удаляется. Настройки
предпросмотра ссылки сохраняются. Сообщения в процессе отправки или с ошибкой
отправки недоступны для редактирования. Ошибка сервера или тайм-аут сохраняет
набранные правки для повтора; текст обновляется после подтверждения Telegram.
При изменении исходника на другом устройстве отмените и откройте редактор заново.
Выход из чата/темы или начало другого сообщения отменяет локальный редактор;
уже отправленный запрос Telegram всё ещё может выполнить.

### WEB proxy / WEB-прокси

Import `https://t.me/webproxy?server=HOST&secret=SECRET` or
`tg://webproxy?server=HOST&secret=SECRET` from a message or QR code, or select
**Settings → Proxy → Type → WEB (HTTPS)**. Enter the hostname (ASCII/IDNA;
`xn--…` for internationalized names), optional base path and secret, then save
and apply. HTTPS/443 is fixed; omit the `port` parameter. A path in a shared link
is URL-encoded, and a path-aware marked secret is decoded during import.

The first Vita implementation supports the reference relay's `https` carrier.
Servers selecting `https-lanes`, `websocket` or `websocket-lanes` report an
unsupported-transport error. The carrier uses native OpenSSL HTTPS with
certificate/hostname verification, without WebKit; it does not reproduce a
browser's TLS fingerprint. It sends already encrypted MTProxy bytes through a
loopback adapter, bounds streams and queues, and never falls back to a direct
Telegram connection after a WEB startup failure. Save and apply again after a
transport failure. Mini Apps and ordinary web pages still connect directly.

Импортируйте `https://t.me/webproxy?server=HOST&secret=SECRET` или
`tg://webproxy?server=HOST&secret=SECRET` из сообщения либо QR-кода. Для ручного
ввода выберите **Настройки → Прокси → Тип → WEB (HTTPS)**, укажите домен
(ASCII/IDNA, для международных имён — `xn--…`), необязательный путь и секрет,
затем сохраните и примените. Используется только HTTPS/443, параметр `port`
в ссылке не нужен. Путь в ссылке кодируется как URL; специальный секрет для
ссылок с путём преобразуется при импорте.

Первая реализация поддерживает транспорт `https` эталонного сервера. Режимы
`https-lanes`, `websocket` и `websocket-lanes` пока дают явную ошибку. HTTPS
работает через OpenSSL с проверкой сертификата и имени сервера, без WebKit;
TLS-отпечаток браузера не воспроизводится. Число потоков и очереди ограничены.
При ошибке запуска WEB прямое подключение к Telegram не включается. После сбоя
транспорта повторите сохранение и применение. Мини-приложения и обычные
веб-страницы продолжают подключаться напрямую.

Host tests cover the wire protocol and a local TLS relay. Real WEB servers,
Vita throughput, suspend/resume and installation still require device checks.
Хост-тесты проверяют протокол и локальный TLS-сервер. Реальные WEB-серверы,
скорость на Vita, сон/возобновление и установку нужно проверить на устройстве.

### Multiple proxies / Несколько прокси

**Settings → Proxy** stores up to 16 MTProxy, SOCKS5, HTTP CONNECT or WEB entries.
The old single-proxy configuration becomes the first entry automatically. Add a
proxy manually or scan a QR code; importing an existing entry selects it without
creating another copy. Cross selects a saved proxy, Square edits it, Triangle
asks to delete it, and Circle returns. The same actions are available by touch.
The outlined row is navigation focus; the filled row with a drawn check is the
selected proxy. Saving runs in the background and temporarily disables edits.

**Check all proxies** measures availability and Telegram round-trip latency one
entry at a time. Measurements are kept in memory. With **Auto-switch on failure**
enabled (the default), background checks run while the proxy connection is in
use. After at least 15 seconds of connection failure, the client picks the
reachable alternative with the lowest recent latency. It waits at least 30
seconds between switches and avoids the failed entry for two minutes. It keeps
a working connection even when another proxy measures faster. Offline state and
resume invalidate the failure timer; old measurements cannot trigger a switch.
When every entry fails, the client keeps using the selected proxy rather than
connecting directly. Automatic choices apply to the current session; the saved
manual preference is tried again after restart. To allow direct networking,
explicitly disable **Use proxy**. Deleting the last entry does not disable it.
WEB still supports only the native `https` carrier described above.

**Настройки → Прокси** хранит до 16 записей MTProxy, SOCKS5, HTTP CONNECT или WEB.
Прежний одиночный прокси автоматически становится первой записью. Добавляйте
прокси вручную или через QR-код; повторный импорт выбирает существующую запись.
Крестик выбирает прокси, квадрат открывает редактирование, треугольник запрашивает
удаление, круг возвращает назад. Эти действия доступны и касанием. Рамка
показывает фокус навигации, заливка с нарисованной галочкой — выбранный прокси.
Список сохраняется в фоне; до завершения записи редактирование недоступно.

**Проверить все прокси** последовательно измеряет доступность и задержку до
Telegram. Результаты хранятся только в памяти. **Автопереключение при сбое**
включено по умолчанию: при использовании прокси выполняются фоновые проверки.
Если соединение не восстанавливается минимум 15 секунд, выбирается доступный
резерв с наименьшей свежей задержкой. Между переключениями выдерживается не
менее 30 секунд, сбойный сервер исключается на две минуты. Работающее соединение
не меняется ради небольшой разницы задержки. При отсутствии сети и после
возобновления таймер сбоя сбрасывается; старые результаты не вызывают переключение.
Если недоступны все записи, прямое соединение не включается. Автоматический выбор
действует в текущем сеансе; после перезапуска снова пробуется сохранённый ручной
выбор. Для прямого подключения явно отключите **Использовать прокси**. Удаление
последней записи само по себе эту настройку не выключает. WEB по-прежнему
поддерживает только нативный транспорт `https`.

On a physical Vita, check controller/touch navigation, the last page, chosen rows
with and without focus, large fonts and light/dark/custom palettes. Also verify
switching during real connection failures, Wi-Fi loss, suspend/resume, active
uploads/downloads and WEB proxy operation. Host tests do not establish these.

### Proxy connection diagnostics / Диагностика подключения прокси (0.39.16)

From 0.39.17 proxy diagnostics start automatically before sign-in, including after a settings reset; no menu action is required. Repeat **Check all** or select and apply a proxy. Wait a few seconds and copy `ux0:data/vitagram/proxy.log` and, if present, `proxy.previous.log`. The **Settings → About and diagnostics → Diagnostic logs → Proxy connections** switch can disable writing for the current session; the next launch enables it again. Each file is capped at 256 KiB; writes run on the Storage worker. Pending events are bounded and `omitted` reports events lost to overflow or write errors. Logs are removed on logout.

Начиная с 0.39.17 журнал прокси включается автоматически до входа в аккаунт, в том числе после сброса настроек; меню не требуется. Повторите **Проверить все** либо выберите и примените прокси. Подождите несколько секунд и скопируйте `ux0:data/vitagram/proxy.log` и, если он есть, `proxy.previous.log`. Переключатель **Настройки → О приложении и диагностика → Диагностические логи → Подключение прокси** отключает запись на текущий сеанс; следующий запуск включает её снова. Каждый файл ограничен 256 КиБ; запись выполняется в потоке Storage. Очередь событий ограничена, `omitted` показывает потери из-за переполнения или ошибок записи. При выходе из аккаунта журналы удаляются.

Only fixed event names and numeric values are recorded. No hostname, IP address, URL, proxy secret, password, bridge/session token, request/response body or account identifier is written. / Записываются только фиксированные названия событий и числа. Имена серверов, IP-адреса, URL, secret, пароли, токены, тела запросов и ответов и идентификаторы аккаунта не записываются.

`id` correlates one application/check with its WEB worker; `at_us` is monotonic time. The initial `apply`/`check` event uses `code` for proxy kind (0 MTProto, 1 SOCKS5, 2 HTTP CONNECT, 3 WEB) and `detail` for the zero-based saved-list index, or -1 for a draft. `queued` and `sent` distinguish application queueing from dispatch to TDLib.

- `dns_system`: system resolver return code; `dns_https`: HTTP status, DNS status in `detail`, and `aux=1` for an absent/invalid IPv4 answer. `dns_cache` marks cached resolution.
- `tcp`: socket `errno` on failure. `tls`: OpenSSL SSL error in `code`, certificate verification result in `detail`, OpenSSL reason code in `aux` (or `errno` for SSL_ERROR_SYSCALL). `ca` uses -1 when the CA store could not be loaded.
- `http_status`: HTTP response status, with a fixed `endpoint` label. Successful stream polls are omitted to keep logging small; errors are retained. `http_io` marks read/write or HTTP parsing failures: -2 oversized headers, -3 invalid response/body limit, -4 invalid/duplicate headers, -5 incomplete headers; nonnegative codes are SSL I/O errors.
- `bootstrap`/`session`: HTTP status and validation success (1/0 in `detail`). `carrier`: 1 HTTPS, 2 HTTPS lanes, 3 WebSocket, 0 other; only HTTPS is supported. `listen` and `downlink` report local listener/thread setup.
- `result`: TDLib error code, latency in milliseconds for checks, and success in `aux` (or success in `detail` for application). `failed` classifies TDLib errors in `detail`: 3 secret, 4 DNS, 5 timeout, 6 connection/socket, 7 authentication, 8 unclassified. Local `failed` codes: 1 incomplete fields, 2 client starting, 10 invalid WEB settings, 11 TLS, 12 DNS, 13 unsupported carrier, 14 WEB protocol/session, 15 worker creation (pthread error in `detail`).
- `timeout`, `cancel`, `retry` and `switch` distinguish timeout, cancellation, retry and automatic failover. `connection` codes are 0 waiting for network, 1 connecting to proxy, 2 connecting, 3 updating, 4 ready, 5 unknown.

DNS/TCP/TLS internals are recorded for native WEB transport. MTProto/SOCKS5/HTTP checks record dispatch and TDLib result/classification; they do not enable TDLib's raw verbose log. Physical Vita network and storage behaviour still requires a device check.

From 0.39.40, Vita TDLib proxy resolution uses asynchronous HTTPS DNS without system-DNS fallback, which can block its single scheduler for 30 seconds after a network failure. If HTTPS DNS is unreachable, resolution fails and can be retried. The shared network pool is 4 MiB; socket writes are capped at 16 KiB and temporary `ENOBUFS` pressure is retried without closing the connection. These measures still require physical Vita upload testing.

Начиная с 0.39.40, TDLib на Vita разрешает имена прокси через асинхронный HTTPS DNS без системного резолвера, способного блокировать сетевой обработчик на 30 секунд после сбоя сети. При недоступном HTTPS DNS запрос завершается ошибкой и может быть повторён. Общий сетевой пул увеличен до 4 МиБ, запись в сокет ограничена порциями по 16 КиБ; временный `ENOBUFS` обрабатывается как пауза без закрытия соединения. Стабильность отправки ещё требует проверки на физической Vita.

TDLib initialization failures use `parameters code=<TDLib code>` and a fixed `detail` classification: 1 API ID, 2 API hash, 3 UTF-8, 4 base64, 5 database/SQLite, 6 database directory, 7 files directory, 8 encryption key, 9 binlog, 10 other. Successful initialization remains `code=0 detail=1`. Local `failed code=16` means a proxy operation is blocked by rejected Telegram initialization; it is not evidence that the proxy itself is unreachable.

For initialization failures, `aux` further classifies known database error messages: 1 cannot open, 2 locked, 3 disk I/O, 4 read-only, 5 malformed/corrupt, 6 not a database, 7 out of memory, 8 missing function, 0 unclassified. The original message and database path are never written.

### Recovery build 0.39.19 / Восстановительная сборка 0.39.19

The native TDLib file database is temporarily disabled, matching 0.39.05, to isolate minute-long internal actor stalls recorded on Vita. The existing SQLite index and its encryption key are retained. In 0.39.19, explicit document-save jobs persisted, but without the native part map an interrupted transfer could download its bytes again after restart. Proxy lists and WEB transport remain available; automatic proxy logging remains enabled.

Файловая база TDLib временно отключена, как в 0.39.05, для проверки многоминутных остановок внутреннего обработчика, зафиксированных на Vita. Существующий индекс SQLite и его ключ шифрования сохраняются. В версии 0.39.19 задания сохранения документов оставались в журнале, но без нативной карты частей незавершённая загрузка после перезапуска могла повторно скачать данные. Список прокси и WEB-транспорт доступны, автозапись журнала прокси включена.

TDLib DNS diagnostics (0.39.21) are written automatically to `proxy.log`. Each
`dns_begin` has its own ID: `code=0` is system DNS and `code=1` is verified HTTPS
DNS; `detail=4/6` is the preferred address family and `aux=1` identifies a lookup
of the HTTPS resolver itself. A matching `dns_system` records the getaddrinfo
result in `code`, errno on failure in `detail`, and duration in microseconds in
`aux`. A matching `dns_https` records the TDLib result code (0 means success),
HTTP status if available, and duration. An unfinished HTTPS lookup ends with
code=-1. These TDLib records use endpoint=dns; no names, IP addresses, credentials
or response bodies are logged. Existing WEB transport records keep their format.

Диагностика DNS TDLib (0.39.21) автоматически пишется в `proxy.log`: отдельные
попытки системного и проверяемого HTTPS DNS получают ID, результат, длительность
и числовые ошибки. Адреса, имена серверов, secret и тела ответов не записываются.
Порядок разрешения имён и выбор прокси диагностика не меняет.

In 0.39.23, TDLib and WEB bootstrap `dns.google:443` through its public anycast
IPv4 address, retaining `dns.google` for TLS verification, SNI and HTTP Host.
This removes the resolver's dependency on system DNS. Google documents the
endpoint and hostname requirements in its [secure transport guide](https://developers.google.com/speed/public-dns/docs/secure-transports).
The numeric bootstrap is logged as `dns_begin code=2` followed by
`dns_bootstrap` for TDLib; WEB uses `dns_bootstrap endpoint=dns`. TDLib HTTPS DNS
errors without a numeric error code now use code=-2; code=-1 remains an
unfinished attempt. In 0.39.21, code=0 alone did not establish success.

В 0.39.23 TDLib и WEB подключают HTTPS-резолвер по его публичному anycast IPv4,
сохраняя проверку сертификата и имя `dns.google` в TLS и HTTP. Системный DNS для
запуска HTTPS-резолвера больше не нужен. Отдельная запись `dns_bootstrap`
показывает этот этап; ошибка HTTPS DNS без числового кода теперь помечается -2.


### Premium emoji / Премиум-эмодзи

When composing a message, the custom keyboard's emoji panel has a **Premium** button. Open an installed
custom emoji pack, then press Cross or tap an emoji to insert it into the draft.
Circle returns to the pack list; Square closes the picker and restores the draft.
The custom emoji ID is preserved when sending mixed text. Telegram decides whether
the account can send the chosen emoji. Messages, captions, drafts and the premium
picker play TGS/WebM emoji at up to 15 fps and 64 px. Up to four distinct visible
emoji animate together; repeated instances share their animation. During scrolling,
typing and main video playback the current frame freezes and the decoder resumes
without reopening the file. Beyond that limit, static previews remain. When Telegram
provides no preview, a static TGS/WebM frame is generated in the image worker.
All visible emoji can load progressively, including large mosaics; full files
download at most four at a time.
The Unicode symbol remains visible until an image is available. Full emoji files
are limited to 1 MiB; inline TGS JSON to 256 KiB and WebM frames to 512×512.
Drafts containing custom emoji stay in the custom keyboard when switching layouts.

В панели эмодзи встроенной клавиатуры нажмите **Премиум**, откройте установленный
набор и выберите эмодзи крестиком или касанием. Кружок возвращает к списку наборов,
квадрат закрывает выбор и возвращает черновик. При отправке сохраняется ID эмодзи;
доступность отправки проверяет Telegram. В сообщениях, подписях, черновике и
выборе премиум-эмодзи проигрываются TGS/WebM до 15 кадров/с с размером до 64 пикселей.
Одновременно анимируются до четырёх разных видимых эмодзи; повторы используют
общую анимацию. При прокрутке, вводе текста и воспроизведении основного видео
текущий кадр замирает, затем декодер продолжает работу без повторного открытия
файла. Сверх лимита остаются статичные превью. Если Telegram не прислал превью,
статичный кадр TGS/WebM создаётся фоновым обработчиком изображений. Все видимые
эмодзи, включая большие мозаики, загружаются постепенно — до четырёх полных файлов
одновременно. Пока изображение недоступно, виден
обычный символ. Полные файлы ограничены 1 МиБ, JSON TGS — 256 КиБ, кадры WebM — 512×512. Черновик с премиум-эмодзи остаётся во встроенной клавиатуре
при переключении раскладки.

### Built-in emoji and draft view / Эмодзи и просмотр черновика

The custom keyboard's smile button opens font-supported Unicode emoji. Use the
category button to cycle groups; hold it to open the category list. Scroll the
list by swiping or using Up/Down, choose with a tap or Cross, and return with
Circle. Use L/R or the arrow buttons for emoji pages, and hold a
tile or Cross for skin-tone variants. Circle or ABC returns to letters without
sending the message. The emoji panel also has Send (Save when editing), disabled
for an empty message. Variants are prepared on the first press and load while the
finger remains down. Recent emoji and chosen variants remain available until the
keyboard closes. The built-in
Noto emoji font stays independent of the selected user text font. Password fields
have neither the emoji picker nor the collapse control.

The lower-left arrow hides the keys and expands the draft to sixteen lines.
The upper-right Circle control beside the language badge closes the keyboard. Swipe or use the directional buttons to scroll. A
quick tap places the cursor and restores the keys; holding for 350 ms opens the
existing themed magnifier and keeps the keys hidden. Release after a swipe or
long hold keeps the expanded view open. Cross also restores the keys. Emoji sequences stay intact during cursor movement and deletion.

Кнопка со смайлом во встроенной клавиатуре открывает эмодзи Unicode, поддержанные
шрифтом. Короткое нажатие кнопки категории переключает группы, удержание открывает
прокручиваемый список. Прокрутка — свайпом или вверх/вниз, выбор — касанием или
Cross, возврат — Circle. L/R и кнопки со стрелками переключают страницы эмодзи.
Удержание плитки или Cross открывает варианты оттенка кожи. Circle или АБВ
возвращает буквы без отправки сообщения. В панели эмодзи есть кнопка «Отправить»
(«Сохранить» при редактировании); пустое сообщение отправить нельзя. Варианты
готовятся при первом нажатии и загружаются, пока палец удерживает экран.
Недавние эмодзи и выбранные варианты доступны до закрытия клавиатуры.
Встроенный шрифт эмодзи Noto не зависит от выбранного пользовательского шрифта
текста. В полях паролей нет выбора эмодзи и кнопки сворачивания.

Нижняя левая стрелка скрывает клавиши и расширяет черновик до шестнадцати строк.
Верхняя правая кнопка Circle рядом с обозначением языка закрывает клавиатуру. Прокрутка — свайпом или кнопками направления.
Короткое касание устанавливает курсор и возвращает клавиши; удержание 350 мс
открывает прежнюю лупу в цветах темы, оставляя клавиши скрытыми. Отпускание после
свайпа или долгого удержания сохраняет расширенный просмотр. Cross тоже
возвращает клавиши. Перемещение курсора и удаление не разделяют
составные эмодзи.


For emoji variants, keep holding after the choices open and slide horizontally
along the key row or into the popup. Release to insert the indicated variant;
move outside the panel to cancel. For large sets, hold at a panel edge to scroll.

Для выбора варианта эмодзи продолжайте удержание после открытия вариантов и
ведите палец вбок вдоль ряда клавиш или внутрь всплывающей панели. Отпустите палец,
чтобы вставить указанный вариант; выход за панель отменяет выбор. В больших
наборах удержание у края прокручивает варианты.

HTTPS DNS transport diagnostics (0.39.25): `dns_begin code=3` identifies a Wget
request, and code=4 identifies its TLS stream. The IDs are separate from the
outer DNS attempt. For endpoint=dns, `tcp code=0` means a socket connect was
initiated, not that the connection completed. `ca` records CA-context creation;
`tls` records stream setup and handshake. Fatal handshake records use the
OpenSSL SSL_ERROR value in code, X509 verification result in detail, and the
OpenSSL reason in aux (errno for SSL_ERROR_SYSCALL). A TLS completion record
has code=0, the verification result in detail and duration in aux; code=-1
means the stream closed before handshake completion. `http_status` records
received status codes; code=-2 indicates a failed request, with last setup phase
in detail: 0 socket, 1 CA, 2 TLS setup, 4 awaiting TLS/HTTP. Redirects finish with
the received status and detail=1. Raw socket failures use id=0, endpoint=control,
`tcp code=<OS error>` and detail=1 pending SO_ERROR, 2 SO_ERROR query failure,
3 read, 4 write. These raw failures cover TDLib sockets generally and cannot
alone identify a proxy. No names, addresses, paths or payloads are included.

Диагностика HTTPS DNS (0.39.25) отдельно отмечает создание сокета, загрузку
сертификатов, настройку и завершение TLS, HTTP-ответы и исходные ошибки сокетов.
Записи содержат только числовые значения. Успешное создание сокета ещё не
означает завершённое TCP-подключение; фактическую работу нужно проверить на Vita.

### Proxy auto-switch delay / Задержка автопереключения прокси

In 0.39.27 the proxy list has an auto-switch checkbox and 5, 10, 15, 30 or
60-second waiting periods (default 10). The value is saved. On the interval row,
Left/Right moves focus and Cross saves; a tap selects the touched interval.
A fill and drawn check show the saved value separately from the focus outline.
The timer applies to sustained connection failure and the next attempt after
an automatic switch. It chooses a recently checked available alternative;
healthy connections stay on their current proxy, offline mode does not switch,
and failed checks can delay selection beyond the chosen interval.

В 0.39.27 в списке прокси есть флажок автопереключения и ожидание 5, 10, 15,
30 или 60 секунд (по умолчанию 10). Значение сохраняется. На строке интервала
влево/вправо перемещает фокус, Cross сохраняет; касание выбирает конкретный
интервал. Заливка с графической галочкой показывает сохранённый вариант,
рамка — фокус. После устойчивого сбоя выбирается проверенный доступный прокси;
работающее соединение сохраняется. При отсутствии сети переключения нет.
Если доступный запасной прокси ещё не найден, ожидание может быть дольше.


Unread mentions / Непрочитанные упоминания: an @ appears beside the unread
indicator in the chat list. While reading away from the end of history, tap the
circular @ to jump to the first unread mention. Visible messages are marked as
read without opening an expanded post. A separate line below the date chip in chat history shows
“{USER} is typing…”.
В списке чатов @ рядом с индикатором непрочитанного обозначает упоминания.
Если история открыта не в конце, круглая кнопка @ переводит к первому
непрочитанному упоминанию. Видимые сообщения учитываются как прочитанные
без открытия развёрнутого поста. Под плашкой даты в истории чата отдельно показывается
«{USER} печатает…».

In 0.39.29 the proxy auto-switch option uses Yes/No text. Its interval row is
shown only while auto-switch is enabled; the saved interval is retained when
the option is turned off.

В 0.39.29 автопереключение прокси отображается через «Да/Нет». Строка интервала
видна только при включённом автопереключении; выбранное время сохраняется после
его отключения.


Settings → Chats → Unread message counter replaces the unread dot with the
full unread count in a glowing circle or oval. The @ mention marker stays
beside it. The switch is off by default and applies to Classic and Combined views.
Настройки → Чаты → Счётчик непрочитанных заменяет точку числом в светящемся
круге или овале. Значок @ остаётся рядом. Переключатель по умолчанию выключен
и действует в классическом и объединённом видах.

Settings → Chats → Muted chats: replace unread badge shows a crossed speaker
instead of unread dots/counts for chats with disabled notifications. @ mentions
stay visible. This display-only switch is off by default; it respects chat
overrides, inherited notification scopes and temporary mute expiry.
Настройки → Чаты → Тихие чаты: значок вместо счётчика показывает перечёркнутый
динамик вместо точки или числа для чатов без уведомлений. @ остаётся видимым.
Настройка по умолчанию выключена и меняет только отображение, учитывая
личные и общие параметры уведомлений и срок временного отключения.
