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

const int PROJECT_ID = 7;
const int LAYOUT = 4;
const wchar_t* APP_TITLE = L"任务7：歌手比赛系统";
const wchar_t* REQUIREMENT_TEXT = L"输入选手信息和十个评委分数，计算去掉最高最低后的平均分，按平均分排序，修改成绩并输出信息。";
const wchar_t* SPECIAL_LABEL = L"计算成绩";
const wchar_t* INPUT_HINT = L"请输入：选手姓名,选手编号,评委1,...,评委10\n例如：张三,S001,90,92,88,91,89,93,90,94,87,92";
const wchar_t* DEFAULT_STATUS = L"已录入";
const wchar_t* STYLE_NAME = L"蒂芙尼蓝评分仪表盘";
const std::vector<std::wstring> FIELD_NAMES = { L"选手姓名", L"选手编号", L"评委1", L"评委2", L"评委3", L"评委4", L"评委5", L"评委6", L"评委7", L"评委8", L"评委9", L"评委10" };

const COLORREF BG_COLOR = RGB(225, 249, 246);
const COLORREF HEADER_COLOR = RGB(128, 209, 200);
const COLORREF ACCENT_COLOR = RGB(0, 116, 110);
const COLORREF PANEL_COLOR = RGB(245, 255, 253);
const COLORREF ROW_COLOR = RGB(255, 255, 255);
const COLORREF SELECTED_COLOR = RGB(205, 241, 236);
const COLORREF LINE_COLOR = RGB(128, 209, 200);
const COLORREF TITLE_COLOR = RGB(0, 0, 0);
const COLORREF TEXT_COLOR = RGB(24, 35, 36);
const COLORREF SUBTEXT_COLOR = RGB(76, 103, 105);

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
    return { Record{1001, { L"张三", L"S001", L"90", L"92", L"88", L"91", L"89", L"93", L"90", L"94", L"87", L"92" }, L"已录入"}, Record{1002, { L"李四", L"S002", L"86", L"88", L"91", L"90", L"89", L"87", L"92", L"90", L"88", L"89" }, L"已录入"} };
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
        if (PROJECT_ID == 7) return {30, 270, 670, 350};
        switch (LAYOUT) {
        case 1: return {220, 104, 860, 350};
        case 2: return {40, 150, 1040, 300};
        case 3: return {420, 150, 660, 475};
        case 4: return {30, 230, 1060, 290};
        default: return {30, 150, 700, 470};
        }
    }

    Rect detailPanel() const {
        if (PROJECT_ID == 7) return {720, 270, 370, 350};
        switch (LAYOUT) {
        case 1: return {220, 474, 860, 150};
        case 2: return {40, 470, 1040, 150};
        case 3: return {30, 150, 360, 475};
        case 4: return {30, 92, 1060, 112};
        default: return {760, 150, 330, 470};
        }
    }

    void drawShell() {
        if (PROJECT_ID == 7) {
            fillRect({0, 0, width, height}, BG_COLOR);
            fillRound({30, 22, 1060, 56}, HEADER_COLOR);
            drawText({52, 35, 520, 28}, APP_TITLE, RGB(0, 0, 0), 24, true, DT_SINGLELINE | DT_END_ELLIPSIS);
            drawText({780, 39, 286, 22}, L"实时评分仪表盘", TITLE_COLOR, 15, true, DT_RIGHT | DT_SINGLELINE);
            drawDashboardOverview();
            return;
        }
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
        if (PROJECT_ID == 7) {
            drawText({52, 94, 430, 24}, REQUIREMENT_TEXT, SUBTEXT_COLOR, 13, false, DT_SINGLELINE | DT_END_ELLIPSIS);
            int x = 510;
            const int y = 90;
            addButton({x, y, 70, 34}, L"新增", Add); x += 78;
            addButton({x, y, 70, 34}, L"修改", Edit); x += 78;
            addButton({x, y, 70, 34}, L"删除", Delete); x += 78;
            addButton({x, y, 92, 34}, SPECIAL_LABEL, Special); x += 100;
            addButton({x, y, 96, 34}, L"成绩排序", Sort); x += 104;
            addButton({x, y, 70, 34}, L"保存", Save); x += 78;
            addButton({x, y, 70, 34}, L"重载", Reload);
            return;
        }
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
        if (PROJECT_ID == 7) {
            drawLeaderboard();
            return;
        }
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

    void drawDashboardOverview() {
        const auto& all = model.all();
        const Record* top = bestSinger();
        const double topScore = top ? Manager::singerAverage(*top) : 0.0;
        const double avgScore = overallAverage();
        const double selectedScore = selectedId >= 0 && model.find(selectedId) ? Manager::singerAverage(*model.find(selectedId)) : 0.0;

        drawMetricCard({30, 140, 250, 106}, L"参赛选手", std::to_wstring(all.size()), L"当前录入总人数");
        drawMetricCard({300, 140, 250, 106}, L"全场均分", fixed2(avgScore), L"按选手平均分计算");
        drawMetricCard({570, 140, 250, 106}, L"最高平均分", fixed2(topScore), top ? safeValue(*top, 0) + L" / " + safeValue(*top, 1) : L"暂无选手");
        drawMetricCard({840, 140, 250, 106}, L"当前选手", selectedId >= 0 ? fixed2(selectedScore) : L"--", selectedId >= 0 && model.find(selectedId) ? safeValue(*model.find(selectedId), 0) : L"点击榜单选择");
    }

    void drawMetricCard(const Rect& rect, const std::wstring& title, const std::wstring& value, const std::wstring& note) {
        fillRound(rect, PANEL_COLOR);
        setlinecolor(LINE_COLOR);
        roundrect(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, 12, 12);
        drawText({rect.x + 18, rect.y + 14, rect.w - 36, 20}, title, SUBTEXT_COLOR, 13, true, DT_SINGLELINE | DT_END_ELLIPSIS);
        drawText({rect.x + 18, rect.y + 38, rect.w - 36, 36}, value, TITLE_COLOR, 28, true, DT_SINGLELINE | DT_END_ELLIPSIS);
        drawText({rect.x + 18, rect.y + 78, rect.w - 36, 18}, note, SUBTEXT_COLOR, 12, false, DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    void drawLeaderboard() {
        Rect panel = listPanel();
        drawPanel(panel, L"成绩排行榜");
        const auto ranked = rankedRecords();
        const int rowH = 46;
        const int pageSize = std::max(1, (panel.h - 112) / rowH);
        const int maxPage = std::max(0, (static_cast<int>(ranked.size()) - 1) / pageSize);
        page = std::clamp(page, 0, maxPage);
        const int start = page * pageSize;

        drawText({panel.x + 24, panel.y + 38, 150, 18}, L"名次 / 选手", SUBTEXT_COLOR, 12, true, DT_SINGLELINE);
        drawText({panel.x + panel.w - 158, panel.y + 38, 120, 18}, L"平均分", SUBTEXT_COLOR, 12, true, DT_RIGHT | DT_SINGLELINE);

        for (int i = 0; i < pageSize && start + i < static_cast<int>(ranked.size()); ++i) {
            const Record* record = ranked[start + i];
            const int rank = start + i + 1;
            const double avg = Manager::singerAverage(*record);
            Rect row{panel.x + 20, panel.y + 62 + i * rowH, panel.w - 40, rowH - 7};
            rows.push_back({row, record->id});
            const bool selected = record->id == selectedId;
            fillRound(row, selected ? SELECTED_COLOR : ROW_COLOR);
            setlinecolor(selected ? ACCENT_COLOR : LINE_COLOR);
            roundrect(row.x, row.y, row.x + row.w, row.y + row.h, 10, 10);

            std::wstring medal = rank == 1 ? L"冠军" : (rank == 2 ? L"亚军" : (rank == 3 ? L"季军" : L"第 " + std::to_wstring(rank) + L" 名"));
            drawText({row.x + 14, row.y + 9, 86, 20}, medal, rank <= 3 ? ACCENT_COLOR : SUBTEXT_COLOR, 13, true, DT_SINGLELINE | DT_END_ELLIPSIS);
            drawText({row.x + 104, row.y + 9, 160, 20}, safeValue(*record, 0) + L"  " + safeValue(*record, 1), TEXT_COLOR, 14, true, DT_SINGLELINE | DT_END_ELLIPSIS);

            Rect bar{row.x + 284, row.y + 14, row.w - 420, 12};
            fillRound(bar, RGB(218, 242, 238));
            int scoreW = static_cast<int>(bar.w * std::clamp(avg, 0.0, 100.0) / 100.0);
            fillRound({bar.x, bar.y, scoreW, bar.h}, scoreColor(avg));
            drawText({row.x + row.w - 116, row.y + 7, 90, 22}, fixed2(avg), TITLE_COLOR, 17, true, DT_RIGHT | DT_SINGLELINE);
        }

        const int pageY = panel.y + panel.h - 38;
        addButton({panel.x + 20, pageY, 78, 30}, L"上一页", Prev);
        addButton({panel.x + 108, pageY, 78, 30}, L"下一页", Next);
        std::wstringstream ss;
        ss << L"第 " << (page + 1) << L" / " << (maxPage + 1) << L" 页，共 " << ranked.size() << L" 名选手";
        drawText({panel.x + 210, pageY + 6, 260, 20}, ss.str(), SUBTEXT_COLOR, 13, false, DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    void drawScoreDetail() {
        Rect panel = detailPanel();
        drawPanel(panel, L"当前选手评分仪表盘");
        const Record* record = model.find(selectedId);
        if (!record) {
            drawText({panel.x + 22, panel.y + 60, panel.w - 44, 54}, L"点击左侧排行榜选择选手，即可查看十位评委分数、去掉最高最低后的平均分和状态。", SUBTEXT_COLOR, 14);
            drawGauge(panel, 0.0, L"--");
            return;
        }

        const double avg = Manager::singerAverage(*record);
        drawText({panel.x + 22, panel.y + 38, panel.w - 44, 24}, safeValue(*record, 0) + L"  " + safeValue(*record, 1), TITLE_COLOR, 18, true, DT_SINGLELINE | DT_END_ELLIPSIS);
        drawGauge(panel, avg, fixed2(avg));
        drawText({panel.x + 235, panel.y + 92, 100, 20}, L"去高低后均分", SUBTEXT_COLOR, 12, false, DT_SINGLELINE | DT_END_ELLIPSIS);

        double minScore = 0.0, maxScore = 0.0;
        scoreRange(*record, minScore, maxScore);
        drawText({panel.x + 22, panel.y + 146, 150, 20}, L"最高分 " + fixed2(maxScore), ACCENT_COLOR, 13, true, DT_SINGLELINE);
        drawText({panel.x + 190, panel.y + 146, 150, 20}, L"最低分 " + fixed2(minScore), SUBTEXT_COLOR, 13, true, DT_SINGLELINE);

        const int startY = panel.y + 178;
        for (int i = 0; i < 10; ++i) {
            const double score = toDouble(safeValue(*record, i + 2));
            const int col = i % 2;
            const int row = i / 2;
            const int x = panel.x + 22 + col * 166;
            const int y = startY + row * 30;
            drawText({x, y, 48, 18}, L"评委" + std::to_wstring(i + 1), SUBTEXT_COLOR, 11, false, DT_SINGLELINE);
            Rect bar{x + 52, y + 5, 76, 9};
            fillRound(bar, RGB(218, 242, 238));
            int w = static_cast<int>(bar.w * std::clamp(score, 0.0, 100.0) / 100.0);
            fillRound({bar.x, bar.y, w, bar.h}, scoreColor(score));
            drawText({x + 132, y - 1, 32, 18}, fixed2(score), TEXT_COLOR, 11, true, DT_RIGHT | DT_SINGLELINE);
        }

        drawText({panel.x + 22, panel.y + panel.h - 30, panel.w - 44, 20}, L"状态：" + record->status + L"    操作：计算成绩 / 成绩排序", SUBTEXT_COLOR, 12, true, DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    void drawGauge(const Rect& panel, double score, const std::wstring& label) {
        Rect box{panel.x + 22, panel.y + 74, 190, 60};
        fillRound(box, RGB(236, 250, 247));
        setlinecolor(LINE_COLOR);
        roundrect(box.x, box.y, box.x + box.w, box.y + box.h, 10, 10);
        Rect bar{box.x + 14, box.y + 38, box.w - 28, 10};
        fillRound(bar, RGB(218, 242, 238));
        int w = static_cast<int>(bar.w * std::clamp(score, 0.0, 100.0) / 100.0);
        fillRound({bar.x, bar.y, w, bar.h}, scoreColor(score));
        drawText({box.x + 14, box.y + 10, box.w - 28, 28}, label, TITLE_COLOR, 27, true, DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    std::vector<const Record*> rankedRecords() const {
        std::vector<const Record*> ranked;
        for (const auto& record : model.all()) ranked.push_back(&record);
        std::sort(ranked.begin(), ranked.end(), [](const Record* a, const Record* b) {
            return Manager::singerAverage(*a) > Manager::singerAverage(*b);
        });
        return ranked;
    }

    const Record* bestSinger() const {
        const auto ranked = rankedRecords();
        return ranked.empty() ? nullptr : ranked.front();
    }

    double overallAverage() const {
        const auto& all = model.all();
        if (all.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& record : all) sum += Manager::singerAverage(record);
        return sum / all.size();
    }

    void scoreRange(const Record& record, double& minScore, double& maxScore) const {
        minScore = 0.0;
        maxScore = 0.0;
        bool initialized = false;
        for (int i = 0; i < 10; ++i) {
            double score = toDouble(safeValue(record, i + 2));
            if (!initialized) {
                minScore = maxScore = score;
                initialized = true;
            } else {
                minScore = std::min(minScore, score);
                maxScore = std::max(maxScore, score);
            }
        }
    }

    COLORREF scoreColor(double score) const {
        if (score >= 92.0) return RGB(0, 116, 110);
        if (score >= 88.0) return RGB(128, 209, 200);
        if (score >= 80.0) return RGB(99, 184, 176);
        return RGB(84, 150, 145);
    }

    void drawDetail() {
        if (PROJECT_ID == 7) {
            drawScoreDetail();
            return;
        }
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
