#include<bits/stdc++.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>

using namespace std;
Fl_Text_Editor *editor = 0;
Fl_Window *window = 0;
Fl_Window* find_dialog = 0;
Fl_Input* find_input = 0;

char filename[256] = "";
int changed = 0;
int loading = 0;

Fl_Text_Buffer *style_buffer = 0;

Fl_Text_Display::Style_Table_Entry style_table[] = {
  { FL_BLACK, FL_COURIER, 14 },           // A - Normal
  { FL_BLUE, FL_COURIER_BOLD, 14 },       // B - Keyword
  { FL_DARK_GREEN, FL_COURIER_ITALIC, 14 },// C - Comment
  { FL_RED, FL_COURIER, 14 },             // D - String
  { FL_MAGENTA, FL_COURIER_BOLD, 14 },    // E - Preprocessor
  { FL_DARK_YELLOW, FL_COURIER, 14 }      // F - Number
};

const char *cpp_keywords[] = {
  "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
  "case", "catch", "char", "class", "compl", "const", "const_cast",
  "continue", "default", "delete", "do", "double", "dynamic_cast",
  "else", "enum", "explicit", "export", "extern", "false", "float",
  "for", "friend", "goto", "if", "inline", "int", "long", "mutable",
  "namespace", "new", "not", "not_eq", "operator", "or", "or_eq",
  "private", "protected", "public", "register", "reinterpret_cast",
  "return", "short", "signed", "sizeof", "static", "static_cast",
  "struct", "switch", "template", "this", "throw", "true", "try",
  "typedef", "typeid", "typename", "union", "unsigned", "using",
  "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
};

void style_parse(const char* text, char* style, int length);
void style_update(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg);

int is_keyword(const char *word) {
    for (unsigned int i = 0; i < sizeof(cpp_keywords) / sizeof(cpp_keywords[0]); i++) {
        if (strcmp(word, cpp_keywords[i]) == 0) return 1;
    }
    return 0;
}

void style_parse(const char* text, char* style, int length) {
    char current_style = 'A';
    for (int i = 0; i < length; i++) {
        if (current_style != 'A' && current_style != 'B' && current_style != 'F') { // If in a multi-line construct
            style[i] = current_style;
            if (current_style == 'C' && text[i] == '*' && i + 1 < length && text[i+1] == '/') { // End of /*...*/ comment
                i++;
                style[i] = 'C';
                current_style = 'A';
            } else if (current_style == 'D' && text[i] == '"') { // End of "..." string
                current_style = 'A';
            }
            continue;
        }

        if (strncmp(text + i, "//", 2) == 0) {
            for(int j=i; j<length; j++) {
                if(text[j] == '\n') { i=j-1; break; }
                style[j] = 'C';
                if (j == length - 1) i = j;
            }
        } else if (strncmp(text + i, "/*", 2) == 0) {
            current_style = 'C';
            style[i++] = 'C';
            style[i] = 'C';
        } else if (text[i] == '"') {
            current_style = 'D';
            style[i] = 'D';
        } else if (isdigit(text[i])) {
            style[i] = 'F';
        } else if (text[i] == '#') {
            style[i] = 'E';
        } else if (isalpha(text[i])) {
            char word[256];
            int j = 0;
            while ((isalnum(text[i + j]) || text[i+j] == '_') && j < 255) {
                word[j] = text[i + j];
                j++;
            }
            word[j] = '\0';
            if (is_keyword(word)) {
                for (int k = 0; k < j; k++) style[i + k] = 'B';
                i += j - 1;
            }
            else {
                for (int k = 0; k < j; k++) style[i + k] = 'A';
                i += j - 1;
            }
        } else {
            style[i] = 'A';
        }
    }
}

void style_update(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg) {
    Fl_Text_Buffer* buffer = (Fl_Text_Buffer*)cbArg;
    int start, end;
    char *style, *text;

    if (nInserted > 0) {
        start = buffer->line_start(pos);
        end = buffer->line_end(pos + nInserted);
    } else {
        start = buffer->line_start(pos);
        end = buffer->line_end(pos);
    }

    text = buffer->text_range(start, end);
    style = style_buffer->text_range(start, end);
    
    style_parse(text, style, end - start);

    style_buffer->replace(start, end, style);
    editor->redisplay_range(start, end);

    free(text);
    free(style);
}

void set_title(Fl_Window * w){
    char title[256];
    if(filename[0] == '\0')
        strcpy(title, "Untitled");
    else{
        char *p = strrchr(filename, '/');
        if(p == NULL) p = filename;
        else p++;
        strcpy(title,p);
    }
    if(changed)
        strcat(title, " *");

    w->label(title);
}

void saveas_cb(Fl_Widget*, void*){
    Fl_File_Chooser fc(".", "*", Fl_File_Chooser::CREATE, "Save File As...");
    fc.show();
    while(fc.shown()){
        Fl::wait();
    }
    if(fc.value() != NULL){
        if(editor->buffer()->savefile(fc.value()))
            fl_alert("Error writing to file: %s", strerror(errno));
        else{
            strcpy(filename, fc.value());
            changed = 0;
            set_title(window);
        }
    }
}

void save_cb(Fl_Widget*,  void*){
    if(filename[0] == '\0'){
        saveas_cb(0,0);
        return;
    }
    else{
        if(editor->buffer()->savefile(filename))
            fl_alert("Error writing to file: %s", strerror(errno));
        else{
            changed = 0;
            set_title(window);
        }
    }
}

int check_save(void){
    if(!changed) return 1;
    int r = fl_choice("The curent file has not been saved.\n"
                    "Do you want to save it?",
                    "Cancel", "Save", "Don't Save");
    if(r == 1){
        save_cb(0,0);
        return !changed;
    }
    return (r == 2)?1:0;
}

void window_cb(Fl_Window* w, void*){
    if(check_save()){
        w->hide();
        exit(0);
    }
}

void quit_cb(Fl_Widget* w, void*){
    window_cb(w->window(), 0);
}

void new_cb(Fl_Widget*, void*){
    if(!check_save()) return;
    filename[0] = '\0';
    editor->buffer()->text("");
    style_buffer->text("");
    changed = 0;
    set_title(window);
}

void open_cb(Fl_Widget*, void*){
    if(!check_save()) return;
    Fl_File_Chooser fc(".", "*", Fl_File_Chooser::SINGLE, "Open File");
    fc.show();
    while(fc.shown()){
        Fl::wait();
    }
    if(fc.value() != NULL){
        loading = 1;
        if(editor->buffer()->loadfile(fc.value())){
            fl_alert("Error reading file: %s", strerror(errno));
        } else {
            strcpy(filename, fc.value());
            set_title(window);
            style_update(0, 0, 0, 0, 0, editor->buffer());
        }
        loading = 0;
        changed = 0;
    }
}

void cancel_find_cb(Fl_Widget* w, void*) {
    find_dialog->hide();
}

void find2_cb(Fl_Widget* w, void* v) {
    const char *search_text = find_input->value();
    if (search_text[0] == '\0') return;
    int start_pos = editor->insert_position();
    int found_pos;
    if (editor->buffer()->search_forward(start_pos, search_text, &found_pos)) {
        editor->buffer()->select(found_pos, found_pos + strlen(search_text));
        editor->insert_position(found_pos + strlen(search_text));
        editor->show_insert_position();
    } else {
        fl_alert("'%s' not found!", search_text);
    }
}

void find_cb(Fl_Widget*, void*){
    if(!find_dialog){
        find_dialog = new Fl_Window(300,105, "Find");
        find_input = new Fl_Input(70, 10, 220, 25, "Find:");
        Fl_Button* find_next = new Fl_Button(100, 70, 90, 25, "Find Next");
        Fl_Button* cancel = new Fl_Button(200, 70, 90, 25, "Cancel");
        find_next->callback((Fl_Callback*)find2_cb);
        cancel->callback((Fl_Callback*)cancel_find_cb);
        find_dialog->end();
    }
    find_dialog->show();
}

void changed_cb(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg){
    if((nInserted || nDeleted) && !loading) changed = 1;
    set_title(window);
    style_update(pos, nInserted, nDeleted, nRestyled, deletedText, cbArg);
}

void cut_cb(Fl_Widget*, void*) {
    Fl_Text_Editor::kf_cut(0, editor);
}
     
void copy_cb(Fl_Widget*, void*) {
    Fl_Text_Editor::kf_copy(0, editor);
}
     
void paste_cb(Fl_Widget*, void*) {
    Fl_Text_Editor::kf_paste(0, editor);
}

void undo_cb(Fl_Widget*, void*){
    editor->buffer()->undo();
}

void word_count_cb(Fl_Widget* w, void* v) {
    char *text = editor->buffer()->text();
    int count = 0;
    bool in_word = false;
    for (int i = 0; text[i]; i++) {
        if (isspace(text[i])) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            count++;
        }
    }
    free(text);
    fl_alert("The document contains %d words.", count);
}

Fl_Menu_Item menuitems[] = {
    { "&File", 0, 0, 0, FL_SUBMENU},
        { "&New File", FL_COMMAND + 'n', (Fl_Callback *)new_cb, 0},
        { "&Open File...", FL_COMMAND + 'o', (Fl_Callback *)open_cb, 0},
        { "&Save File", FL_COMMAND + 's', (Fl_Callback *)save_cb, 0},
        { "Save File &As...", FL_COMMAND + FL_SHIFT + 's', (Fl_Callback *)saveas_cb, 0},
        { "E&xit", FL_COMMAND + 'q', (Fl_Callback *)quit_cb,0},
        {0},
    { "&Edit", 0, 0, 0, FL_SUBMENU},
        { "&Cut", FL_COMMAND + 'x', (Fl_Callback *)cut_cb, 0},
        { "&Copy", FL_COMMAND + 'c', (Fl_Callback *)copy_cb, 0},
        { "&Paste", FL_COMMAND + 'v', (Fl_Callback *)paste_cb, 0},
        { "&Undo", FL_COMMAND + 'z', (Fl_Callback *)undo_cb, 0},
        {0},
    { "&Search", 0,0,0, FL_SUBMENU},
        {"&Find...", FL_COMMAND + 'f', (Fl_Callback *)find_cb, 0},
        {0},
    { "&Tools", 0, 0, 0, FL_SUBMENU},
        { "&Word Count", 0, (Fl_Callback *)word_count_cb, 0},
        {0},
    {0}
};

int main(int argc, char *argv[]) {
    window = new Fl_Window(640, 480);
    window->callback((Fl_Callback *)window_cb);

    Fl_Menu_Bar *menu = new Fl_Menu_Bar(0, 0, 640, 30);
    menu->menu(menuitems);

    editor = new Fl_Text_Editor(0, 30, 640, 450);

    Fl_Text_Buffer* text_buf = new Fl_Text_Buffer();
    style_buffer = new Fl_Text_Buffer();
    editor->buffer(text_buf);

    editor->highlight_data(
        style_buffer,
        style_table,
        sizeof(style_table) / sizeof(style_table[0]),
        'A', 0, 0
    );

    text_buf->add_modify_callback(changed_cb, text_buf);

    window->end();
    set_title(window);
    window->show(argc, argv);
    return Fl::run();
}