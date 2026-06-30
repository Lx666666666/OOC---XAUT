#define UNICODE
#define _UNICODE

#include <graphics.h>
#include <windows.h>

#include <algorithm>
#include <codecvt>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

const int PROJECT_ID = 8;
const int LAYOUT = 2;
const wchar_t* APP_TITLE = L"任务8：活期储蓄账目管理系统";
const wchar_t* REQUIREMENT_TEXT = L"用户注册、登录、存款、取款和删除用户。";
const wchar_t* SPECIAL_LABEL = L"存取款";
const wchar_t* INPUT_HINT = L"请输入：用户名,账号,密码,余额\n例如：张三,A001,123456,500";
const wchar_t* DEFAULT_STATUS = L"正常";
const wchar_t* STYLE_NAME = L"银行沉稳金绿风";
const std::vector<std::wstring> FIELD_NAMES = { L"用户名", L"账号", L"密码", L"余额" };

const COLORREF BG_COLOR = RGB(235, 241, 236);
const COLORREF HEADER_COLOR = RGB(21, 87, 65);
const COLORREF ACCENT_COLOR = RGB(184, 138, 54);
const COLORREF PANEL_COLOR = RGB(255, 255, 250);
const COLORREF ROW_COLOR = RGB(250, 253, 249);
const COLORREF SELECTED_COLOR = RGB(232, 246, 231);
const COLORREF LINE_COLOR = RGB(199, 215, 202);
const COLORREF TITLE_COLOR = RGB(24, 81, 63);
const COLORREF TEXT_COLOR = RGB(39, 55, 45);
const COLORREF SUBTEXT_COLOR = RGB(88, 108, 93);

struct Rect {
    int x{};
    int y{};
    int w{};
    int h{};
    bool contains(int px, int py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
};

struct Button {
    Rect rect;
    std::wstring label;
    int action{};
};

struct Record {
    int id{};
    std::vector<std::wstring> values;
    std::wstring status;
};

class TextCodec {
public:
    static std::string toUtf8(const std::wstring& value) {
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(value);
    }
    static std::wstring fromUtf8(const std::string& value) {
        static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(value);
    }
};

static std::wstring trim(const std::wstring& value) {
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

static std::wstring normalizeInput(std::wstring text) {
    std::replace(text.begin(), text.end(), L'，', L',');
    std::replace(text.begin(), text.end(), L'|', L'/');
    std::replace(text.begin(), text.end(), L'\r', L' ');
    std::replace(text.begin(), text.end(), L'\n', L' ');
    return text;
}

static std::vector<std::wstring> splitWide(const std::wstring& text, wchar_t delim) {
    std::vector<std::wstring> parts;
    std::wstringstream ss(text);
    std::wstring part;
    while (std::getline(ss, part, delim)) parts.push_back(trim(part));
    return parts;
}

static std::vector<std::string> splitNarrow(const std::string& text, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(text);
    std::string part;
    while (std::getline(ss, part, delim)) parts.push_back(part);
    return parts;
}

static double toDouble(const std::wstring& value, double fallback = 0.0) {
    try { return std::stod(trim(value)); } catch (...) { return fallback; }
}

static std::wstring money(double value) {
    std::wstringstream ss;
    ss << L"¥" << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

static std::wstring fixed2(double value) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

static std::vector<Record> seedRecords() {
    return { Record{1001, { L"张三", L"A001", L"123456", L"500" }, L"正常"}, Record{1002, { L"李四", L"A002", L"888888", L"1200" }, L"正常"} };
}

class Manager {
public:
    void load(const fs::path& dir) {
        dataDir = dir;
        fs::create_directories(dataDir);
        records.clear();
        std::ifstream file(dataDir / "records.csv", std::ios::binary);
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            auto parts = splitNarrow(line, '|');
            if (parts.size() < FIELD_NAMES.size() + 2) continue;
            try {
                Record r;
                r.id = std::stoi(parts[0]);
                for (size_t i = 0; i < FIELD_NAMES.size(); ++i) r.values.push_back(TextCodec::fromUtf8(parts[i + 1]));
                r.status = TextCodec::fromUtf8(parts[FIELD_NAMES.size() + 1]);
                records.push_back(r);
            } catch (...) {
            }
        }
        if (records.empty()) {
            records = seedRecords();
            save();
        }
        sortById();
    }

    void save() const {
        fs::create_directories(dataDir);
        std::ofstream file(dataDir / "records.csv", std::ios::binary | std::ios::trunc);
        for (const auto& r : records) {
            file << r.id;
            for (const auto& value : r.values) file << '|' << TextCodec::toUtf8(value);
            file << '|' << TextCodec::toUtf8(r.status) << '\n';
        }
    }

    const std::vector<Record>& all() const { return records; }

    Record* find(int id) {
        for (auto& r : records) if (r.id == id) return &r;
        return nullptr;
    }

    const Record* find(int id) const {
        for (const auto& r : records) if (r.id == id) return &r;
        return nullptr;
    }

    int nextId() const {
        int id = 1001;
        for (const auto& r : records) id = std::max(id, r.id + 1);
        return id;
    }

    void add(std::vector<std::wstring> values) {
        normalizeValues(values);
        records.push_back(Record{nextId(), values, DEFAULT_STATUS});
        sortById();
    }

    void update(int id, std::vector<std::wstring> values) {
        normalizeValues(values);
        if (Record* r = find(id)) r->values = values;
    }

    void remove(int id) {
        records.erase(std::remove_if(records.begin(), records.end(), [id](const Record& r) { return r.id == id; }), records.end());
    }

    void sortById() {
        std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) { return a.id < b.id; });
    }

    void sortSingerByAverage() {
        std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
            return singerAverage(a) > singerAverage(b);
        });
    }

    static double singerAverage(const Record& r) {
        if (r.values.size() < 12) return 0.0;
        std::vector<double> scores;
        for (size_t i = 2; i < 12; ++i) scores.push_back(toDouble(r.values[i]));
        if (scores.size() < 3) return 0.0;
        auto minmax = std::minmax_element(scores.begin(), scores.end());
        double sum = 0.0;
        for (double score : scores) sum += score;
        return (sum - *minmax.first - *minmax.second) / 8.0;
    }

private:
    fs::path dataDir;
    std::vector<Record> records;

    static void normalizeValues(std::vector<std::wstring>& values) {
        while (values.size() < FIELD_NAMES.size()) values.push_back(L"");
        if (values.size() > FIELD_NAMES.size()) values.resize(FIELD_NAMES.size());
        for (auto& value : values) value = normalizeInput(value);
    }
};

class GuiApp {
public:
    explicit GuiApp(Manager& manager) : model(manager) {}

    static fs::path dataPath() { return executableDir() / "data"; }

    void run() {
        initgraph(width, height, EX_DBLCLKS);
        setbkcolor(BG_COLOR);
        setbkmode(TRANSPARENT);
        BeginBatchDraw();
        bool running = true;
        while (running) {
            draw();
            ExMessage msg{};
            while (peekmessage(&msg, EX_MOUSE | EX_KEY)) {
                if (msg.message == WM_LBUTTONDOWN) handleClick(msg.x, msg.y);
                else if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) running = false;
            }
            Sleep(16);
        }
        EndBatchDraw();
        closegraph();
    }

private:
    static constexpr int width = 1120;
    static constexpr int height = 700;
    Manager& model;
    int selectedId = -1;
    int page = 0;
    std::wstring status = L"系统已启动。数据会自动保存到 data/records.csv。";
    std::vector<Button> buttons;
    std::vector<std::pair<Rect, int>> rows;

    enum Action { Add = 1, Edit, Delete, Special, Sort, Prev, Next, Save, Reload };

    void draw() {
        buttons.clear();
        rows.clear();
        cleardevice();
        drawShell();
        drawToolbar();
        drawList();
        drawDetail();
        drawStatus();
        FlushBatchDraw();
    }

    Rect listPanel() const {
        switch (LAYOUT) {
        case 1: return {220, 104, 860, 350};
        case 2: return {40, 150, 1040, 300};
        case 3: return {420, 150, 660, 475};
        case 4: return {30, 230, 1060, 290};
        default: return {30, 150, 700, 470};
        }
    }

    Rect detailPanel() const {
        switch (LAYOUT) {
        case 1: return {220, 474, 860, 150};
        case 2: return {40, 470, 1040, 150};
        case 3: return {30, 150, 360, 475};
        case 4: return {30, 92, 1060, 112};
        default: return {760, 150, 330, 470};
        }
    }

    void drawShell() {
        if (LAYOUT == 1) {
            fillRect({0, 0, 190, height}, HEADER_COLOR);
            drawText({24, 28, 140, 70}, APP_TITLE, RGB(255, 255, 255), 24, true);
            drawText({24, 110, 138, 60}, STYLE_NAME, RGB(224, 246, 239), 14);
            drawText({220, 24, 640, 38}, REQUIREMENT_TEXT, SUBTEXT_COLOR, 16);
            return;
        }
        if (LAYOUT == 2) {
            fillRect({0, 0, width, height}, BG_COLOR);
            fillRect({0, 0, width, 78}, HEADER_COLOR);
            setlinecolor(ACCENT_COLOR);
            line(40, 78, 1080, 78);
            drawText({40, 22, 650, 34}, APP_TITLE, TITLE_COLOR, 26, true);
            drawText({770, 27, 310, 28}, STYLE_NAME, SUBTEXT_COLOR, 14, false, DT_RIGHT | DT_SINGLELINE);
            drawText({40, 100, 850, 34}, REQUIREMENT_TEXT, SUBTEXT_COLOR, 15);
            return;
        }
        if (LAYOUT == 4) {
            fillRect({0, 0, width, height}, BG_COLOR);
            fillRound({30, 24, 1060, 48}, HEADER_COLOR);
            drawText({52, 35, 660, 26}, APP_TITLE, RGB(255, 255, 255), 22, true);
            drawText({780, 37, 290, 22}, STYLE_NAME, RGB(255, 243, 220), 13, false, DT_RIGHT | DT_SINGLELINE);
            return;
        }
        fillRect({0, 0, width, height}, BG_COLOR);
        fillRect({0, 0, width, 72}, HEADER_COLOR);
        drawText({30, 18, 660, 34}, APP_TITLE, RGB(255, 255, 255), 26, true);
        drawText({760, 23, 320, 24}, STYLE_NAME, RGB(226, 236, 255), 14, false, DT_RIGHT | DT_SINGLELINE);
        drawText({30, 90, 710, 42}, REQUIREMENT_TEXT, SUBTEXT_COLOR, 16);
    }

    void drawToolbar() {
        if (LAYOUT == 1) {
            int y = 208;
            addButton({24, y, 140, 34}, L"新增", Add); y += 44;
            addButton({24, y, 140, 34}, L"修改", Edit); y += 44;
            addButton({24, y, 140, 34}, L"删除", Delete); y += 44;
            addButton({24, y, 140, 34}, SPECIAL_LABEL, Special); y += 44;
            if (PROJECT_ID == 7) { addButton({24, y, 140, 34}, L"成绩排序", Sort); y += 44; }
            addButton({24, y, 140, 34}, L"保存", Save); y += 44;
            addButton({24, y, 140, 34}, L"重载", Reload);
            return;
        }
        if (LAYOUT == 3) {
            addButton({30, 92, 70, 34}, L"新增", Add);
            addButton({110, 92, 70, 34}, L"修改", Edit);
            addButton({190, 92, 70, 34}, L"删除", Delete);
            addButton({270, 92, 110, 34}, SPECIAL_LABEL, Special);
            addButton({420, 92, 70, 34}, L"保存", Save);
            addButton({500, 92, 70, 34}, L"重载", Reload);
            if (PROJECT_ID == 7) addButton({580, 92, 100, 34}, L"成绩排序", Sort);
            return;
        }
        if (LAYOUT == 4) {
            drawText({52, 102, 950, 40}, REQUIREMENT_TEXT, SUBTEXT_COLOR, 15);
            int y = 540;
            addButton({30, y, 86, 34}, L"新增", Add);
            addButton({126, y, 86, 34}, L"修改", Edit);
            addButton({222, y, 86, 34}, L"删除", Delete);
            addButton({318, y, 120, 34}, SPECIAL_LABEL, Special);
            addButton({448, y, 86, 34}, L"保存", Save);
            addButton({544, y, 86, 34}, L"重载", Reload);
            if (PROJECT_ID == 7) addButton({640, y, 110, 34}, L"成绩排序", Sort);
            return;
        }
        int y = LAYOUT == 2 ? 96 : 92;
        int x = LAYOUT == 2 ? 800 : 770;
        addButton({x, y, 70, 34}, L"新增", Add);
        addButton({x + 78, y, 70, 34}, L"修改", Edit);
        addButton({x + 156, y, 70, 34}, L"删除", Delete);
        addButton({x + 234, y, 110, 34}, SPECIAL_LABEL, Special);
        addButton({x, y + 42, 70, 34}, L"保存", Save);
        addButton({x + 78, y + 42, 70, 34}, L"重载", Reload);
        if (PROJECT_ID == 7) addButton({x + 156, y + 42, 100, 34}, L"成绩排序", Sort);
    }

    void drawList() {
        Rect panel = listPanel();
        drawPanel(panel, L"记录列表");
        const auto& all = model.all();
        const int rowH = LAYOUT == 4 ? 86 : 36;
        const int top = panel.y + 58;
        const int pageSize = LAYOUT == 4 ? 6 : std::max(1, (panel.h - 112) / rowH);
        const int maxPage = std::max(0, (static_cast<int>(all.size()) - 1) / pageSize);
        page = std::clamp(page, 0, maxPage);
        const int start = page * pageSize;

        if (LAYOUT != 4) {
            drawText({panel.x + 20, panel.y + 34, panel.w - 40, 20}, tableHeader(), SUBTEXT_COLOR, 13, true, DT_SINGLELINE | DT_END_ELLIPSIS);
        }

        for (int i = 0; i < pageSize && start + i < static_cast<int>(all.size()); ++i) {
            const Record& record = all[start + i];
            Rect row = LAYOUT == 4 ? cardRect(panel, i) : Rect{panel.x + 20, top + i * rowH, panel.w - 40, rowH - 5};
            rows.push_back({row, record.id});
            const bool selected = record.id == selectedId;
            fillRound(row, selected ? SELECTED_COLOR : ROW_COLOR);
            setlinecolor(selected ? ACCENT_COLOR : LINE_COLOR);
            roundrect(row.x, row.y, row.x + row.w, row.y + row.h, 8, 8);
            if (LAYOUT == 4) {
                drawText({row.x + 16, row.y + 10, row.w - 32, 22}, shortTitle(record), TITLE_COLOR, 16, true, DT_SINGLELINE | DT_END_ELLIPSIS);
                drawText({row.x + 16, row.y + 38, row.w - 32, 20}, rowText(record), SUBTEXT_COLOR, 13, false, DT_SINGLELINE | DT_END_ELLIPSIS);
            } else {
                drawText({row.x + 12, row.y + 6, row.w - 24, 20}, rowText(record), TEXT_COLOR, 13, false, DT_SINGLELINE | DT_END_ELLIPSIS);
            }
        }

        const int pageY = panel.y + panel.h - 42;
        addButton({panel.x + 20, pageY, 78, 30}, L"上一页", Prev);
        addButton({panel.x + 108, pageY, 78, 30}, L"下一页", Next);
        std::wstringstream ss;
        ss << L"第 " << (page + 1) << L" / " << (maxPage + 1) << L" 页，共 " << all.size() << L" 条";
        drawText({panel.x + 210, pageY + 6, 260, 20}, ss.str(), SUBTEXT_COLOR, 13);
    }

    Rect cardRect(const Rect& panel, int i) const {
        int col = i % 2;
        int row = i / 2;
        int cw = (panel.w - 58) / 2;
        return {panel.x + 20 + col * (cw + 18), panel.y + 58 + row * 76, cw, 66};
    }

    void drawDetail() {
        Rect panel = detailPanel();
        drawPanel(panel, L"当前记录详情");
        const Record* record = model.find(selectedId);
        if (!record) {
            drawText({panel.x + 22, panel.y + 55, panel.w - 44, panel.h - 70}, L"未选择记录。点击列表中的一项即可查看或修改。", SUBTEXT_COLOR, 15);
            return;
        }

        drawText({panel.x + 22, panel.y + 36, panel.w - 44, 22}, L"编号：" + std::to_wstring(record->id) + L"    " + computedSummary(*record), ACCENT_COLOR, 15, true, DT_SINGLELINE | DT_END_ELLIPSIS);
        int columns = panel.w > 760 ? 3 : (panel.w > 520 ? 2 : 1);
        int cellW = (panel.w - 44) / columns;
        int startY = panel.y + 68;
        int lineH = 26;
        for (size_t i = 0; i < FIELD_NAMES.size(); ++i) {
            int col = static_cast<int>(i % columns);
            int row = static_cast<int>(i / columns);
            int x = panel.x + 22 + col * cellW;
            int y = startY + row * lineH;
            if (y + lineH > panel.y + panel.h - 30) break;
            drawText({x, y, cellW - 12, 20}, FIELD_NAMES[i] + L"：" + safeValue(*record, i), TEXT_COLOR, 13, false, DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        drawText({panel.x + 22, panel.y + panel.h - 30, panel.w - 44, 20}, L"状态：" + record->status, SUBTEXT_COLOR, 13, true, DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    void drawStatus() {
        Rect bar = LAYOUT == 1 ? Rect{220, 642, 860, 34} : Rect{30, 642, 1060, 34};
        fillRound(bar, PANEL_COLOR);
        setlinecolor(LINE_COLOR);
        roundrect(bar.x, bar.y, bar.x + bar.w, bar.y + bar.h, 8, 8);
        drawText({bar.x + 18, bar.y + 8, bar.w - 36, 20}, status, SUBTEXT_COLOR, 13, false, DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    void handleClick(int x, int y) {
        for (const auto& row : rows) {
            if (row.first.contains(x, y)) {
                selectedId = row.second;
                status = L"已选择记录：" + std::to_wstring(selectedId);
                return;
            }
        }
        for (const auto& button : buttons) {
            if (button.rect.contains(x, y)) {
                handleAction(button.action);
                return;
            }
        }
    }

    void handleAction(int action) {
        switch (action) {
        case Add: addDialog(); break;
        case Edit: editDialog(); break;
        case Delete: deleteDialog(); break;
        case Special: specialAction(); break;
        case Sort:
            model.sortSingerByAverage();
            model.save();
            status = L"已按平均分从高到低排序。";
            break;
        case Prev: page = std::max(0, page - 1); break;
        case Next: ++page; break;
        case Save:
            model.save();
            status = L"保存成功。";
            break;
        case Reload:
            model.load(dataPath());
            status = L"已重新加载数据。";
            break;
        }
    }

    void addDialog() {
        std::wstring input;
        if (!prompt(L"新增记录", INPUT_HINT, input)) return;
        model.add(splitWide(normalizeInput(input), L','));
        selectedId = model.nextId() - 1;
        model.save();
        status = L"新增成功。";
    }

    void editDialog() {
        const Record* record = model.find(selectedId);
        if (!record) {
            status = L"请先选择要修改的记录。";
            return;
        }
        std::wstring input;
        if (!prompt(L"修改记录", INPUT_HINT, input, joinValues(*record))) return;
        model.update(record->id, splitWide(normalizeInput(input), L','));
        model.save();
        status = L"修改成功。";
    }

    void deleteDialog() {
        if (!model.find(selectedId)) {
            status = L"请先选择要删除的记录。";
            return;
        }
        std::wstring confirm;
        if (prompt(L"删除确认", L"输入 YES 确认删除当前记录。", confirm) && (confirm == L"YES" || confirm == L"yes")) {
            model.remove(selectedId);
            selectedId = -1;
            model.save();
            status = L"删除成功。";
        }
    }

    void specialAction() {
        Record* record = model.find(selectedId);
        if (!record) {
            status = L"请先选择一条记录。";
            return;
        }
        switch (PROJECT_ID) {
        case 0: {
            const std::wstring no = safeValue(*record, 3);
            int count = 0;
            for (const auto& item : model.all()) if (safeValue(item, 3) == no) ++count;
            status = safeValue(*record, 2) + L" 当前共有缺课记录 " + std::to_wstring(count) + L" 条。";
            break;
        }
        case 1:
            record->status = L"已出院";
            status = safeValue(*record, 0) + L" 已办理出院。";
            break;
        case 2:
            record->status = L"已结账";
            status = safeValue(*record, 0) + L" 网费：" + money(internetFee(*record));
            break;
        case 3:
            record->status = L"已计算";
            status = safeValue(*record, 0) + L" 应缴费用：" + money(utilityFee(*record));
            break;
        case 4: {
            const std::wstring no = safeValue(*record, 1);
            int count = 0;
            for (const auto& item : model.all()) if (safeValue(item, 1) == no) ++count;
            status = safeValue(*record, 0) + L" 当前选课 " + std::to_wstring(count) + L" 门。";
            break;
        }
        case 5:
            status = safeValue(*record, 0) + L" 电话：" + safeValue(*record, 1) + L" 邮箱：" + safeValue(*record, 3);
            break;
        case 6:
            status = safeValue(*record, 0) + L" 月薪：" + money(toDouble(safeValue(*record, 3)));
            break;
        case 7:
            status = safeValue(*record, 0) + L" 平均分：" + fixed2(Manager::singerAverage(*record));
            break;
        case 8:
            savingsDialog(*record);
            break;
        case 9: {
            const std::wstring room = safeValue(*record, 2) + L"-" + safeValue(*record, 3);
            int count = 0;
            for (const auto& item : model.all()) if (safeValue(item, 2) + L"-" + safeValue(item, 3) == room) ++count;
            status = room + L" 当前入住 " + std::to_wstring(count) + L" 人。";
            break;
        }
        }
        model.save();
    }

    void savingsDialog(Record& record) {
        std::wstring input;
        if (!prompt(L"存取款", L"输入金额：正数为存款，负数为取款。\n例如：100 或 -50", input, L"100")) return;
        const double amount = toDouble(input);
        double balance = toDouble(safeValue(record, 3));
        if (balance + amount < 0) {
            status = L"取款失败：余额不足。";
            return;
        }
        balance += amount;
        record.values[3] = fixed2(balance);
        status = safeValue(record, 0) + L" 当前余额：" + money(balance);
    }

    bool prompt(const std::wstring& title, const std::wstring& message, std::wstring& output, const std::wstring& defaultValue = L"") {
        wchar_t buffer[1024]{};
        wcsncpy_s(buffer, defaultValue.c_str(), _TRUNCATE);
        if (InputBox(buffer, 1024, message.c_str(), title.c_str(), defaultValue.c_str(), 500, 210, false)) {
            output = trim(buffer);
            return !output.empty();
        }
        return false;
    }

    std::wstring tableHeader() const {
        std::wstring h = L"编号";
        for (size_t i = 0; i < std::min<size_t>(4, FIELD_NAMES.size()); ++i) h += L"  |  " + FIELD_NAMES[i];
        h += L"  |  状态";
        return h;
    }

    std::wstring shortTitle(const Record& record) const {
        std::wstring s = std::to_wstring(record.id);
        if (!record.values.empty()) s += L"  " + record.values[0];
        if (record.values.size() > 1) s += L"  " + record.values[1];
        return s;
    }

    std::wstring rowText(const Record& record) const {
        std::wstringstream ss;
        ss << record.id;
        const size_t shown = std::min<size_t>(4, record.values.size());
        for (size_t i = 0; i < shown; ++i) ss << L"  |  " << record.values[i];
        ss << L"  |  " << record.status << L"  |  " << computedSummary(record);
        return ss.str();
    }

    std::wstring computedSummary(const Record& record) const {
        switch (PROJECT_ID) {
        case 2: return L"费用 " + money(internetFee(record));
        case 3: return L"应缴 " + money(utilityFee(record));
        case 6: return L"工资 " + money(toDouble(safeValue(record, 3)));
        case 7: return L"平均分 " + fixed2(Manager::singerAverage(record));
        case 8: return L"余额 " + money(toDouble(safeValue(record, 3)));
        default: return L"状态 " + record.status;
        }
    }

    static double internetFee(const Record& record) {
        const std::wstring type = safeValue(record, 1);
        const double hours = toDouble(safeValue(record, 3));
        const bool member = type.find(L"会员") != std::wstring::npos && type.find(L"非") == std::wstring::npos;
        return hours * (member ? 3.0 : 5.0);
    }

    static double utilityFee(const Record& record) {
        return toDouble(safeValue(record, 2)) * 3.5 + toDouble(safeValue(record, 3)) * 0.6 + toDouble(safeValue(record, 4)) * 2.8;
    }

    static std::wstring safeValue(const Record& record, size_t index) {
        return index < record.values.size() ? record.values[index] : L"";
    }

    static std::wstring joinValues(const Record& record) {
        std::wstring text;
        for (size_t i = 0; i < record.values.size(); ++i) {
            if (i) text += L",";
            text += record.values[i];
        }
        return text;
    }

    void addButton(const Rect& rect, const std::wstring& label, int action) {
        buttons.push_back(Button{rect, label, action});
        COLORREF fill = (LAYOUT == 2 || LAYOUT == 7) ? HEADER_COLOR : PANEL_COLOR;
        fillRound(rect, fill);
        setlinecolor(ACCENT_COLOR);
        roundrect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, 8, 8);
        drawText(rect, label, LAYOUT == 2 ? TITLE_COLOR : TEXT_COLOR, 13, true, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    void drawPanel(const Rect& rect, const std::wstring& title) {
        fillRound(rect, PANEL_COLOR);
        setlinecolor(LINE_COLOR);
        roundrect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, 10, 10);
        drawText({rect.x + 20, rect.y + 15, rect.w - 40, 26}, title, TITLE_COLOR, 19, true);
        setlinecolor(ACCENT_COLOR);
        line(rect.x + 20, rect.y + 47, rect.x + rect.w - 20, rect.y + 47);
    }

    void fillRect(const Rect& rect, COLORREF color) {
        setfillcolor(color);
        solidrectangle(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    }

    void fillRound(const Rect& rect, COLORREF color) {
        setfillcolor(color);
        solidroundrect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, 8, 8);
    }

    void drawText(const Rect& rect, const std::wstring& text, COLORREF color, int size, bool bold = false, UINT format = DT_LEFT | DT_TOP | DT_WORDBREAK) {
        settextcolor(color);
        settextstyle(size, 0, L"Microsoft YaHei UI", 0, 0, bold ? FW_BOLD : FW_NORMAL, false, false, false);
        RECT r{rect.x, rect.y, rect.x + rect.w, rect.y + rect.h};
        drawtext(text.c_str(), &r, format);
    }

    static fs::path executableDir() {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        return fs::path(path).parent_path();
    }
};

int main() {
    Manager manager;
    manager.load(GuiApp::dataPath());
    GuiApp app(manager);
    app.run();
    manager.save();
    return 0;
}
