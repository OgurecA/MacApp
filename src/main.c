#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/stat.h>
#include <unistd.h>
#include <json-glib/json-glib.h>
#include <time.h>

GtkWidget *image;
GtkWidget *file_list;
GtkWidget *selected_label = NULL;
GtkWidget *update_button;
GtkWidget *select_button;
GtkWidget *back_button;
GtkWidget *change_button;
GtkWidget *open_button;
GtkWidget *reset_search_button;
GtkWidget *add_extra_file_button;
GtkWidget *search_list;
GtkWidget *reset_button;
gboolean in_file_view = FALSE;


char selected_folder_name[256] = "";
char selected_file_name[256] = "";

typedef struct {
    char *name;
    char *profession;
    char *phone;
    char *telegram;
    char *whatsapp;
    char *website;
    char *email;
    char *country;
    char *city;
    char *description;
} FilterState;

FilterState current_filters;

typedef struct {
    GtkWidget *name_entry;
    GtkWidget *profession_entry;
    GtkWidget *phone_entry;
    GtkWidget *telegram_entry;
    GtkWidget *whatsapp_entry;
    GtkWidget *email_entry;
    GtkWidget *website_entry;
    GtkWidget *description_view;
    GtkWidget *country_entry;
    GtkWidget *city_entry;
    GtkWidget *date_entry;
} FieldWidgets;

FieldWidgets field_widgets;

typedef struct {
    GtkWidget *name_entry;
    GtkWidget *profession_entry;
    GtkWidget *phone_entry;
    GtkWidget *telegram_entry;
    GtkWidget *whatsapp_entry;
    GtkWidget *email_entry;
    GtkWidget *website_entry;
    GtkWidget *description_view;
    GtkWidget *country_entry;
    GtkWidget *city_entry;
    GtkWidget *date_entry;
} FieldWidgetsSearch;

FieldWidgetsSearch field_widgets_search;

#define GET_JSON_STR(o, key) (json_object_has_member((o), (key)) ?           \
                              json_object_get_string_member((o), (key)) : "")


char *str_replace_all(const char *str, const char *old, const char *new_str) {
    GString *result = g_string_new("");
    const char *pos = str;
    size_t old_len = strlen(old);

    while (*pos) {
        const char *match = strstr(pos, old);
        if (match) {
            g_string_append_len(result, pos, match - pos);
            g_string_append(result, new_str);
            pos = match + old_len;
        } else {
            g_string_append(result, pos);
            break;
        }
    }

    return g_string_free(result, FALSE); // не освобождает память
}

void on_creation_file_delete_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    GtkListBoxRow *row = GTK_LIST_BOX_ROW(g_object_get_data(G_OBJECT(menuitem), "row"));
    if (!row) return;
    gtk_widget_destroy(GTK_WIDGET(row));
}

gboolean on_creation_listbox_right_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(widget), (gint)event->y);
        if (!row) return FALSE;

        GtkWidget *box = gtk_bin_get_child(GTK_BIN(row));
        GList *children = gtk_container_get_children(GTK_CONTAINER(box));
        GtkWidget *label = NULL;

        for (GList *i = children; i != NULL; i = i->next) {
            if (GTK_IS_LABEL(i->data)) {
                label = GTK_WIDGET(i->data);
                break;
            }
        }
        g_list_free(children);

        if (!label) return FALSE;

        GtkWidget *menu = gtk_menu_new();
        GtkWidget *delete_item = gtk_menu_item_new_with_label("Удалить");

        g_object_set_data(G_OBJECT(delete_item), "row", row);
        g_signal_connect(delete_item, "activate", G_CALLBACK(on_creation_file_delete_clicked), widget);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);
        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
        return TRUE;
    }
    return FALSE;
}

// Добавление файла в список с кнопкой удаления и сохранением пути
void add_file_to_list(const char *filepath) {
    const char *basename = g_path_get_basename(filepath);

    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

    GtkWidget *label = gtk_label_new(basename);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

    // Сохраняем оригинальный путь в label
    g_object_set_data_full(G_OBJECT(label), "filepath", g_strdup(filepath), g_free);

    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_add(GTK_CONTAINER(row), content_box);

    gtk_box_pack_start(GTK_BOX(content_box), label, TRUE, TRUE, 0);

    gtk_list_box_insert(GTK_LIST_BOX(file_list), row, -1);
}

void refresh_portfolio_list(GtkWidget *target_list) {
    // Очистка текущего списка
    GList *children = gtk_container_get_children(GTK_CONTAINER(target_list));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    // Путь к каталогу портфолио
    const char *folder_path = "Portfolios";

    // Проверим, существует ли каталог
    if (!g_file_test(folder_path, G_FILE_TEST_IS_DIR)) {
        g_warning("Каталог '%s' не найден.", folder_path);
        return;
    }

    // Открытие каталога и добавление всех подкаталогов (портфолио)
    GDir *dir = g_dir_open(folder_path, 0, NULL);
    if (dir) {
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            char full_path[600];
            snprintf(full_path, sizeof(full_path), "%s/%s", folder_path, filename);

            if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *label = gtk_label_new(filename);
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);
                gtk_container_add(GTK_CONTAINER(row), label);
                gtk_list_box_insert(GTK_LIST_BOX(target_list), row, -1);
            }
        }
        g_dir_close(dir);
    }

    gtk_widget_show_all(target_list);
}

void clear_creation_fields(void) {
    if (!GTK_IS_ENTRY(field_widgets_search.name_entry) ||
        !GTK_IS_ENTRY(field_widgets_search.profession_entry) ||
        !GTK_IS_ENTRY(field_widgets_search.phone_entry) ||
        !GTK_IS_ENTRY(field_widgets_search.telegram_entry) ||
        !GTK_IS_ENTRY(field_widgets_search.whatsapp_entry) ||
        !GTK_IS_ENTRY(field_widgets_search.website_entry) ||
        !GTK_IS_TEXT_VIEW(field_widgets_search.description_view) ||
        !GTK_IS_LIST_BOX(file_list)) {
        g_warning("Некоторые поля формы недействительны");
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(field_widgets_search.name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets_search.profession_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets_search.phone_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets_search.telegram_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets_search.whatsapp_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets_search.website_entry), "");

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(field_widgets_search.description_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    if (image && GTK_IS_IMAGE(image)) {
        gtk_image_clear(GTK_IMAGE(image));
    }

    GList *children = gtk_container_get_children(GTK_CONTAINER(file_list));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

// Сохранение скриншота и копирование файлов
void on_save_clicked(GtkWidget *button, gpointer data) {
    GtkWidget *window = GTK_WIDGET(data);
    GtkWidget *name_entry = g_object_get_data(G_OBJECT(button), "name_entry");
    GtkWidget *profession_entry = g_object_get_data(G_OBJECT(button), "profession_entry");
    GtkWidget *phone_entry = g_object_get_data(G_OBJECT(button), "phone_entry");
    GtkWidget *telegram_entry = g_object_get_data(G_OBJECT(button), "telegram_entry");
    GtkWidget *whatsapp_entry = g_object_get_data(G_OBJECT(button), "whatsapp_entry");
    GtkWidget *email_entry = g_object_get_data(G_OBJECT(button), "email_entry");
    GtkWidget *website_entry = g_object_get_data(G_OBJECT(button), "website_entry");
    GtkWidget *description_view = g_object_get_data(G_OBJECT(button), "description_view");
    GtkWidget *country_entry = g_object_get_data(G_OBJECT(button), "country_entry");
    GtkWidget *city_entry = g_object_get_data(G_OBJECT(button), "city_entry");
    GtkWidget *date_entry = g_object_get_data(G_OBJECT(button), "date_entry");

    const char *name = gtk_entry_get_text(GTK_ENTRY(name_entry));
    const char *profession = gtk_entry_get_text(GTK_ENTRY(profession_entry));

    if (!name || g_strcmp0(name, "") == 0 || !profession || g_strcmp0(profession, "") == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Введите имя и профессию перед сохранением.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    char full_name[512];
    snprintf(full_name, sizeof(full_name), "%s_%s", name, profession);

    char folder_path[512];
    snprintf(folder_path, sizeof(folder_path), "Portfolios/%s", full_name);
    mkdir("Portfolios", 0755);
    mkdir(folder_path, 0755);

    // Сохраняем JSON
    const char *phone = gtk_entry_get_text(GTK_ENTRY(phone_entry));
    const char *telegram = gtk_entry_get_text(GTK_ENTRY(telegram_entry));
    const char *whatsapp = gtk_entry_get_text(GTK_ENTRY(whatsapp_entry));
    const char *website = gtk_entry_get_text(GTK_ENTRY(website_entry));
    const char *email = gtk_entry_get_text(GTK_ENTRY(email_entry));
    const char *country = gtk_entry_get_text(GTK_ENTRY(country_entry));
    const char *city = gtk_entry_get_text(GTK_ENTRY(city_entry));
    const char *date = gtk_entry_get_text(GTK_ENTRY(date_entry));

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(description_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *description = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    char json_path[600];
    snprintf(json_path, sizeof(json_path), "%s/%s.json", folder_path, full_name);

    FILE *json_file = fopen(json_path, "w");
    if (json_file) {
        fprintf(json_file, "{\n");
        fprintf(json_file, "  \"name\": \"%s\",\n", name);
        fprintf(json_file, "  \"profession\": \"%s\",\n", profession);
        fprintf(json_file, "  \"phone\": \"%s\",\n", phone);
        fprintf(json_file, "  \"telegram\": \"%s\",\n", telegram);
        fprintf(json_file, "  \"whatsapp\": \"%s\",\n", whatsapp);
        fprintf(json_file, "  \"website\": \"%s\",\n", website);
        fprintf(json_file, "  \"email\": \"%s\",\n", email);
        fprintf(json_file, "  \"country\": \"%s\",\n", country);
        fprintf(json_file, "  \"city\": \"%s\",\n", city);
        fprintf(json_file, "  \"date\": \"%s\",\n", date);
        // Экранируем переносы строк
        char *escaped_description = str_replace_all(description, "\n", "\\n");
        fprintf(json_file, "  \"description\": \"%s\"\n", escaped_description);
        g_free(escaped_description);
        fprintf(json_file, "}\n");
        fclose(json_file);
    }
    g_free(description);

    // Скриншот с обрезкой сверху/снизу
    GdkWindow *gdk_window = gtk_widget_get_window(window);
    int width = gdk_window_get_width(gdk_window);
    int height = gdk_window_get_height(gdk_window);
    GdkPixbuf *full = gdk_pixbuf_get_from_window(gdk_window, 0, 0, width, height);
    GdkPixbuf *cropped = NULL;
    if (full) {
        int crop_top = 45;
        int crop_bottom = 45;
        int crop_height = height - crop_top - crop_bottom;
        if (crop_height > 0) {
            cropped = gdk_pixbuf_new_subpixbuf(full, 0, crop_top, width, crop_height);
        }
    }

    if (cropped) {
        char file_path[600];
        snprintf(file_path, sizeof(file_path), "%s/%s.png", folder_path, full_name);
        gdk_pixbuf_save(cropped, file_path, "png", NULL, NULL);
        g_object_unref(cropped);
    }
    if (full) {
        g_object_unref(full);
    }

    // Копирование файлов из списка
    GList *children = gtk_container_get_children(GTK_CONTAINER(file_list));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        GtkWidget *row_box = gtk_bin_get_child(GTK_BIN(row));
        GList *items = gtk_container_get_children(GTK_CONTAINER(row_box));

        for (GList *i = items; i != NULL; i = i->next) {
            GtkWidget *label = GTK_WIDGET(i->data);
            if (GTK_IS_LABEL(label)) {
                const char *src_path = g_object_get_data(G_OBJECT(label), "filepath");
                if (src_path) {
                    const char *basename = g_path_get_basename(src_path);
                    char dest_path[600];
                    snprintf(dest_path, sizeof(dest_path), "%s/%s", folder_path, basename);

                    GFile *src = g_file_new_for_path(src_path);
                    GFile *dest = g_file_new_for_path(dest_path);
                    g_file_copy(src, dest, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
                    g_object_unref(src);
                    g_object_unref(dest);
                }
            }
        }
        g_list_free(items);
    }
    g_list_free(children);

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Портфолио сохранено в: %s", folder_path);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    refresh_portfolio_list(search_list);
    clear_creation_fields();

}

// Остальные функции
void on_choose_photo_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Выбор изображения", GTK_WINDOW(data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Отмена", GTK_RESPONSE_CANCEL,
        "_Открыть", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(filename, 256, 256, TRUE, NULL);
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
            g_object_unref(pixbuf);
        }
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void on_add_file_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Добавить файл", GTK_WINDOW(data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Отмена", GTK_RESPONSE_CANCEL,
        "_Добавить", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        for (GSList *iter = files; iter != NULL; iter = iter->next) {
            add_file_to_list((const char *)iter->data);
            g_free(iter->data);
        }
        g_slist_free(files);
        gtk_widget_show_all(file_list);
    }

    gtk_widget_destroy(dialog);
}

void on_search_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    if (row) {
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(row));
        if (GTK_IS_LABEL(child)) {
            const gchar *text = gtk_label_get_text(GTK_LABEL(child));
            strncpy(selected_file_name, text, sizeof(selected_file_name));
            selected_file_name[sizeof(selected_file_name) - 1] = '\0';
        }
    }
}

void on_file_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    if (!in_file_view) return;  // Только если в режиме просмотра файлов

    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    if (!GTK_IS_LABEL(label)) return;

    const char *file_name = gtk_label_get_text(GTK_LABEL(label));
    if (!file_name || selected_folder_name[0] == '\0') return;

    char full_path[600];
    snprintf(full_path, sizeof(full_path), "Portfolios/%s/%s", selected_folder_name, file_name);

    char command[700];
    snprintf(command, sizeof(command), "xdg-open \"%s\"", full_path);
    g_spawn_command_line_async(command, NULL);
}

gboolean on_listbox_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (!in_file_view) return FALSE;

    if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
        GtkListBox *box = GTK_LIST_BOX(widget);
        GtkListBoxRow *row = gtk_list_box_get_row_at_y(box, (gint)event->y);
        if (row) {
            on_file_row_activated(box, row, NULL);
            return TRUE;
        }
    }

    return FALSE;
}

void load_json_to_fields(const char *folder_name) {
    char json_path[600];
    snprintf(json_path, sizeof(json_path), "Portfolios/%s/%s.json", folder_name, folder_name);

    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    if (!json_parser_load_from_file(parser, json_path, &error)) {
        g_warning("Failed to load JSON: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_warning("JSON root is not an object");
        g_object_unref(parser);
        return;
    }

    JsonObject *obj = json_node_get_object(root);

    #define SET_ENTRY_FROM_JSON(key, widget) \
        if (json_object_has_member(obj, key)) { \
            const char *val = json_object_get_string_member(obj, key); \
            gtk_entry_set_text(GTK_ENTRY(widget), val); \
        }

    SET_ENTRY_FROM_JSON("name", field_widgets.name_entry);
    SET_ENTRY_FROM_JSON("profession", field_widgets.profession_entry);
    SET_ENTRY_FROM_JSON("phone", field_widgets.phone_entry);
    SET_ENTRY_FROM_JSON("telegram", field_widgets.telegram_entry);
    SET_ENTRY_FROM_JSON("whatsapp", field_widgets.whatsapp_entry);
    SET_ENTRY_FROM_JSON("website", field_widgets.website_entry);
    SET_ENTRY_FROM_JSON("email", field_widgets.email_entry);
    SET_ENTRY_FROM_JSON("country", field_widgets.country_entry);
    SET_ENTRY_FROM_JSON("city", field_widgets.city_entry);
    SET_ENTRY_FROM_JSON("date", field_widgets.date_entry);

    if (json_object_has_member(obj, "description")) {
        const char *desc = json_object_get_string_member(obj, "description");
        char *unescaped = str_replace_all(desc, "\\n", "\n");
    
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(field_widgets.description_view));
        gtk_text_buffer_set_text(buffer, unescaped, -1);
    
        g_free(unescaped);
    }
    

    g_object_unref(parser);
}

void save_current_filters() {
    g_free(current_filters.name);
    g_free(current_filters.profession);
    g_free(current_filters.phone);
    g_free(current_filters.telegram);
    g_free(current_filters.whatsapp);
    g_free(current_filters.website);
    g_free(current_filters.email);
    g_free(current_filters.country);
    g_free(current_filters.city);
    g_free(current_filters.description);

    current_filters.name = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.name_entry)));
    current_filters.profession = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.profession_entry)));
    current_filters.phone = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.phone_entry)));
    current_filters.telegram = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.telegram_entry)));
    current_filters.whatsapp = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.whatsapp_entry)));
    current_filters.website = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.website_entry)));
    current_filters.email = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.email_entry)));
    current_filters.country = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.country_entry)));
    current_filters.city = g_strdup(gtk_entry_get_text(GTK_ENTRY(field_widgets.city_entry)));

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(field_widgets.description_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    current_filters.description = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
}

void refresh_file_list_in_selected_folder(GtkWidget *file_list_widget) {
    // Очистить текущий список
    GList *children = gtk_container_get_children(GTK_CONTAINER(file_list_widget));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    if (selected_folder_name[0] == '\0') return;

    char folder_path[600];
    snprintf(folder_path, sizeof(folder_path), "Portfolios/%s", selected_folder_name);

    GDir *dir = g_dir_open(folder_path, 0, NULL);
    if (!dir) return;

    char png_to_hide[300];
    char json_to_hide[300];
    snprintf(png_to_hide, sizeof(png_to_hide), "%s.png", selected_folder_name);
    snprintf(json_to_hide, sizeof(json_to_hide), "%s.json", selected_folder_name);

    const gchar *filename;
    while ((filename = g_dir_read_name(dir)) != NULL) {
        // Пропускаем "Имя.png" и "Имя.json"
        if (g_strcmp0(filename, png_to_hide) == 0 || g_strcmp0(filename, json_to_hide) == 0) {
            continue;
        }

        // Добавление оставшихся файлов в список
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(filename);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(file_list_widget), row, -1);
    }

    g_dir_close(dir);
    gtk_widget_show_all(file_list_widget);
}

void on_select_button_clicked(GtkWidget *button, gpointer data) {
    GtkWidget *target_list = GTK_WIDGET(data);
    in_file_view = TRUE;


    if (strlen(selected_file_name) == 0) return;

    char folder_path[600];
    snprintf(folder_path, sizeof(folder_path), "Portfolios/%s", selected_file_name);
    snprintf(selected_folder_name, sizeof(selected_folder_name), "%s", selected_file_name);

    // Очищаем список
    GList *children = gtk_container_get_children(GTK_CONTAINER(target_list));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    gtk_widget_hide(select_button);
    gtk_widget_show(open_button);
    gtk_widget_show(back_button);
    gtk_widget_show(change_button);
    gtk_widget_hide(update_button);
    gtk_widget_show(add_extra_file_button);
    gtk_widget_hide(reset_search_button);


    // Загружаем файлы
    GDir *dir = g_dir_open(folder_path, 0, NULL);
    if (dir) {
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            char full_path[600];
            snprintf(full_path, sizeof(full_path), "%s/%s", folder_path, filename);

            if (g_file_test(full_path, G_FILE_TEST_IS_REGULAR)) {
                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *label = gtk_label_new(filename);
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);
                gtk_container_add(GTK_CONTAINER(row), label);
                gtk_list_box_insert(GTK_LIST_BOX(target_list), row, -1);
            }
        }
        g_dir_close(dir);
    }

    gtk_widget_show_all(target_list);
    load_json_to_fields(selected_file_name);
    refresh_file_list_in_selected_folder(search_list);
    
}

void on_open_button_clicked(GtkWidget *button, gpointer data) {
    if (selected_file_name[0] == '\0') return;

    char folder_path[600];
    snprintf(folder_path, sizeof(folder_path), "Portfolios/%s", selected_folder_name);

    // Команда для открытия папки в файловом менеджере
    char command[700];
    snprintf(command, sizeof(command), "xdg-open \"%s\"", folder_path);

    g_spawn_command_line_async(command, NULL);
}

void filter_portfolios_by_fields(GtkWidget *list_box) {
    const char *name_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.name_entry));
    const char *profession_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.profession_entry));
    const char *phone_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.phone_entry));
    const char *telegram_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.telegram_entry));
    const char *whatsapp_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.whatsapp_entry));
    const char *website_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.website_entry));
    const char *email_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.email_entry));
    const char *country_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.country_entry));
    const char *city_filter = gtk_entry_get_text(GTK_ENTRY(field_widgets.city_entry));
    
    GtkTextBuffer *desc_buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(field_widgets.description_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(desc_buf, &start, &end);
    char *description_filter = gtk_text_buffer_get_text(desc_buf, &start, &end, FALSE);

    GList *children = gtk_container_get_children(GTK_CONTAINER(list_box));
    for (GList *iter = children; iter != NULL; iter = iter->next)
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    GDir *dir = g_dir_open("Portfolios", 0, NULL);
    if (!dir) {
        g_free(description_filter);
        return;
    }

    const gchar *folder;
    while ((folder = g_dir_read_name(dir)) != NULL) {
        char json_path[600];
        snprintf(json_path, sizeof(json_path), "Portfolios/%s/%s.json", folder, folder);

        JsonParser *parser = json_parser_new();
        GError *error = NULL;
        if (!json_parser_load_from_file(parser, json_path, &error)) {
            g_clear_error(&error);
            g_object_unref(parser);
            continue;
        }

        JsonObject *obj = json_node_get_object(json_parser_get_root(parser));
        const char *name       = GET_JSON_STR(obj, "name");
        const char *profession = GET_JSON_STR(obj, "profession");
        const char *phone      = GET_JSON_STR(obj, "phone");
        const char *telegram   = GET_JSON_STR(obj, "telegram");
        const char *whatsapp   = GET_JSON_STR(obj, "whatsapp");
        const char *website    = GET_JSON_STR(obj, "website");
        const char *email      = GET_JSON_STR(obj, "email");      // → теперь безопасно
        const char *country    = GET_JSON_STR(obj, "country");
        const char *city       = GET_JSON_STR(obj, "city");
        const char *description= GET_JSON_STR(obj, "description");

        gboolean match = TRUE;

        if (name_filter[0] && !g_str_has_prefix(g_ascii_strdown(name, -1), g_ascii_strdown(name_filter, -1)))
            match = FALSE;
        if (profession_filter[0] && !g_str_has_prefix(g_ascii_strdown(profession, -1), g_ascii_strdown(profession_filter, -1)))
            match = FALSE;
        if (phone_filter[0] && !g_str_has_prefix(g_ascii_strdown(phone, -1), g_ascii_strdown(phone_filter, -1)))
            match = FALSE;
        if (telegram_filter[0] && !g_str_has_prefix(g_ascii_strdown(telegram, -1), g_ascii_strdown(telegram_filter, -1)))
            match = FALSE;
        if (whatsapp_filter[0] && !g_str_has_prefix(g_ascii_strdown(whatsapp, -1), g_ascii_strdown(whatsapp_filter, -1)))
            match = FALSE;
        if (website_filter[0] && !g_str_has_prefix(g_ascii_strdown(website, -1), g_ascii_strdown(website_filter, -1)))
            match = FALSE;
        if (email_filter[0] && !g_str_has_prefix(g_ascii_strdown(email, -1), g_ascii_strdown(email_filter, -1)))
            match = FALSE;
        if (country_filter[0] && !g_str_has_prefix(g_ascii_strdown(country, -1), g_ascii_strdown(country_filter, -1)))
            match = FALSE;
        if (city_filter[0] && !g_str_has_prefix(g_ascii_strdown(city, -1), g_ascii_strdown(city_filter, -1)))
            match = FALSE;


            if (description_filter[0]) {
                char *desc_text = g_ascii_strdown(description, -1);
                char *filter_text = g_ascii_strdown(description_filter, -1);
            
                gboolean desc_match = FALSE;
            
                // Разбиваем фильтр на слова
                gchar **filter_words = g_strsplit(filter_text, " ", -1);
            
                for (int i = 0; filter_words[i] != NULL; i++) {
                    if (strlen(filter_words[i]) == 0) continue;
            
                    if (g_strrstr(desc_text, filter_words[i]) != NULL) {
                        desc_match = TRUE;
                        break;
                    }
                }
            
                g_strfreev(filter_words);
                g_free(desc_text);
                g_free(filter_text);
            
                if (!desc_match)
                    match = FALSE;
            }
            
        if (match) {
            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *label = gtk_label_new(folder);
            gtk_label_set_xalign(GTK_LABEL(label), 0.0);
            gtk_container_add(GTK_CONTAINER(row), label);
            gtk_list_box_insert(GTK_LIST_BOX(list_box), row, -1);
        }

        g_object_unref(parser);
    }

    g_dir_close(dir);
    gtk_widget_show_all(list_box);
    g_free(description_filter);
}

void on_back_button_clicked(GtkWidget *button, gpointer user_data) {
    GtkWidget *search_list = GTK_WIDGET(user_data);
    in_file_view = FALSE;


    // Очистить список
    GList *children = gtk_container_get_children(GTK_CONTAINER(search_list));
    for (GList *iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    // Заново загрузить папки
    GDir *dir = g_dir_open("Portfolios", 0, NULL);
    if (dir) {
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            char full_path[600];
            snprintf(full_path, sizeof(full_path), "Portfolios/%s", filename);

            if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *label = gtk_label_new(filename);
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);
                gtk_container_add(GTK_CONTAINER(row), label);
                gtk_list_box_insert(GTK_LIST_BOX(search_list), row, -1);
            }
        }
        g_dir_close(dir);
    }

    gtk_widget_show_all(search_list);

    // Показать/скрыть кнопки
    gtk_widget_show(select_button);
    gtk_widget_hide(change_button);
    gtk_widget_hide(open_button);
    gtk_widget_hide(back_button);
    gtk_widget_show(update_button);
    gtk_widget_hide(add_extra_file_button);
    gtk_widget_show(reset_search_button);

    // Очистить текущий выбор
    selected_file_name[0] = '\0';
    selected_folder_name[0] = '\0';

    // Вставляем сохранённые фильтры обратно в поля
    gtk_entry_set_text(GTK_ENTRY(field_widgets.name_entry), current_filters.name);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.profession_entry), current_filters.profession);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.phone_entry), current_filters.phone);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.telegram_entry), current_filters.telegram);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.whatsapp_entry), current_filters.whatsapp);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.website_entry), current_filters.website);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.email_entry), current_filters.email);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.country_entry), current_filters.country);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.city_entry), current_filters.city);
    gtk_entry_set_text(GTK_ENTRY(field_widgets.date_entry), "");

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(field_widgets.description_view));
    gtk_text_buffer_set_text(buffer, current_filters.description, -1);

    filter_portfolios_by_fields(search_list);
    
}

void on_change_button_clicked(GtkWidget *button, gpointer user_data) {
    if (selected_folder_name[0] == '\0') return;

    // Получаем значения из полей
    const char *name = gtk_entry_get_text(GTK_ENTRY(field_widgets.name_entry));
    const char *profession = gtk_entry_get_text(GTK_ENTRY(field_widgets.profession_entry));
    const char *phone = gtk_entry_get_text(GTK_ENTRY(field_widgets.phone_entry));
    const char *telegram = gtk_entry_get_text(GTK_ENTRY(field_widgets.telegram_entry));
    const char *whatsapp = gtk_entry_get_text(GTK_ENTRY(field_widgets.whatsapp_entry));
    const char *website = gtk_entry_get_text(GTK_ENTRY(field_widgets.website_entry));
    const char *email = gtk_entry_get_text(GTK_ENTRY(field_widgets.email_entry));
    const char *country = gtk_entry_get_text(GTK_ENTRY(field_widgets.country_entry));
    const char *city = gtk_entry_get_text(GTK_ENTRY(field_widgets.city_entry));
    const char *date = gtk_entry_get_text(GTK_ENTRY(field_widgets.date_entry));

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(field_widgets.description_view));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *description = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

    // Экранируем переносы строк
    char *escaped_description = str_replace_all(description, "\n", "\\n");

    // Путь к JSON
    char json_path[600];
    snprintf(json_path, sizeof(json_path), "Portfolios/%s/%s.json", selected_folder_name, selected_folder_name);

    FILE *json_file = fopen(json_path, "w");
    if (json_file) {
        fprintf(json_file, "{\n");
        fprintf(json_file, "  \"name\": \"%s\",\n", name);
        fprintf(json_file, "  \"profession\": \"%s\",\n", profession);
        fprintf(json_file, "  \"phone\": \"%s\",\n", phone);
        fprintf(json_file, "  \"telegram\": \"%s\",\n", telegram);
        fprintf(json_file, "  \"whatsapp\": \"%s\",\n", whatsapp);
        fprintf(json_file, "  \"website\": \"%s\",\n", website);
        fprintf(json_file, "  \"email\": \"%s\",\n", email);
        fprintf(json_file, "  \"country\": \"%s\",\n", country);
        fprintf(json_file, "  \"city\": \"%s\",\n", city);
        fprintf(json_file, "  \"date\": \"%s\",\n", date);
        fprintf(json_file, "  \"description\": \"%s\"\n", escaped_description);
        fprintf(json_file, "}\n");
        fclose(json_file);
    }

    g_free(escaped_description);
    g_free(description);

    GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                               "Данные успешно обновлены.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_delete_file_clicked(GtkMenuItem *menuitem, gpointer user_data) {
    const char *filename = g_object_get_data(G_OBJECT(menuitem), "filename");
    if (!filename || selected_folder_name[0] == '\0') return;

    GtkWidget *search_list = GTK_WIDGET(user_data);

    // Убедимся, что это действительно папка
    char folder_check[600];
    snprintf(folder_check, sizeof(folder_check), "Portfolios/%s", selected_folder_name);
    if (!g_file_test(folder_check, G_FILE_TEST_IS_DIR)) {
        g_warning("'%s' is not a directory. Skipping reload.", selected_folder_name);
        return;
    }

    // Удаление файла
    char filepath[600];
    snprintf(filepath, sizeof(filepath), "%s/%s", folder_check, filename);

    if (remove(filepath) == 0) {
        g_print("Файл удалён: %s\n", filepath);
    } else {
        g_warning("Не удалось удалить файл: %s\n", filepath);
    }

    // Очистим имя выбранного файла
    selected_file_name[0] = '\0';


    g_print("Selected Folder: %s\n", selected_folder_name);
    // Обновим отображение файлов
    refresh_file_list_in_selected_folder(search_list);

}

gboolean on_listbox_right_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) { // правая кнопка
        GtkListBoxRow *row = gtk_list_box_get_row_at_y(GTK_LIST_BOX(widget), (gint)event->y);
        if (!row) return FALSE;

        GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
        if (!GTK_IS_LABEL(label)) return FALSE;

        const char *filename = gtk_label_get_text(GTK_LABEL(label));
        if (!filename || selected_folder_name[0] == '\0') return FALSE;

        GtkWidget *menu = gtk_menu_new();
        GtkWidget *delete_item = gtk_menu_item_new_with_label("Удалить");

        // передаем копию имени файла
        char *file_copy = g_strdup(filename);
        g_object_set_data_full(G_OBJECT(delete_item), "filename", file_copy, g_free);

        g_signal_connect(delete_item, "activate", G_CALLBACK(on_delete_file_clicked), user_data);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), delete_item);
        gtk_widget_show_all(menu);

        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
        return TRUE;
    }

    return FALSE;
}

void on_add_extra_file_clicked(GtkWidget *widget, gpointer user_data) {
    if (selected_folder_name[0] == '\0') {
        g_warning("Папка не выбрана.");
        return;
    }

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Выберите файл",
        GTK_WINDOW(user_data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Отмена", GTK_RESPONSE_CANCEL,
        "_Открыть", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *src_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        const char *filename = g_path_get_basename(src_path);

        char dest_path[600];
        snprintf(dest_path, sizeof(dest_path), "Portfolios/%s/%s", selected_folder_name, filename);

        if (g_file_test(dest_path, G_FILE_TEST_EXISTS)) {
            g_warning("Файл уже существует: %s", dest_path);
        } else {
            if (g_file_copy(g_file_new_for_path(src_path),
                            g_file_new_for_path(dest_path),
                            G_FILE_COPY_NONE,
                            NULL, NULL, NULL, NULL)) {
                g_print("Файл скопирован: %s\n", dest_path);
            } else {
                g_warning("Ошибка при копировании файла.");
            }
        }

        g_free(src_path);
    }

    gtk_widget_destroy(dialog);

    // Обновляем список файлов
    refresh_file_list_in_selected_folder(search_list);
}

void on_reset_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *list_box = GTK_WIDGET(user_data);

    // Очистить поля
    gtk_entry_set_text(GTK_ENTRY(field_widgets.name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.profession_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.phone_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.telegram_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.whatsapp_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.website_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.email_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.country_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.city_entry), "");
    gtk_entry_set_text(GTK_ENTRY(field_widgets.date_entry), "");

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(field_widgets.description_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    filter_portfolios_by_fields(list_box);
    save_current_filters();
}

void on_update_button_clicked(GtkButton *button, gpointer user_data) {
    GtkWidget *list_box = GTK_WIDGET(user_data);
    filter_portfolios_by_fields(list_box);
    save_current_filters();
}


int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    const gchar *custom_css = 
    "textview {\n"
    "    background-color: #1e1e1e;\n"
    "    color: #ffffff;\n"
    "    border: 1px solid #555;\n"
    "    border-radius: 4px;\n"
    "    padding: 6px;\n"
    "}\n"
    "textview > text {\n"
    "    background-color: transparent;\n"
    "    color: #ffffff;\n"
    "    padding: 4px;\n"
    "}\n"
    "list {\n"
    "    background-color: #1e1e1e;\n"
    "    color: #ffffff;\n"
    "    border-radius: 4px;\n"
    "}\n";

    GtkCssProvider *provider = gtk_css_provider_new();
gtk_css_provider_load_from_data(provider, custom_css, -1, NULL);

GdkScreen *screen = gdk_screen_get_default();
gtk_style_context_add_provider_for_screen(screen,
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_USER);
g_object_unref(provider);


    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Документ пользователя");
    gtk_window_set_default_size(GTK_WINDOW(window), 900, 600);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Создаём notebook
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    // Страница "Создание"
    GtkWidget *creation_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(creation_page), 10);

    GtkWidget *top_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(creation_page), top_hbox, FALSE, FALSE, 0);

    GtkWidget *left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(top_hbox), left_vbox, FALSE, FALSE, 0);

    image = gtk_image_new();
    gtk_widget_set_size_request(image, 256, 256);
    gtk_box_pack_start(GTK_BOX(left_vbox), image, FALSE, FALSE, 0);

    GtkWidget *choose_button = gtk_button_new_with_label("Выбрать фото");
    g_signal_connect(choose_button, "clicked", G_CALLBACK(on_choose_photo_clicked), window);
    gtk_box_pack_start(GTK_BOX(left_vbox), choose_button, FALSE, FALSE, 0);

    GtkWidget *add_file_button = gtk_button_new_with_label("Добавить файл");
    g_signal_connect(add_file_button, "clicked", G_CALLBACK(on_add_file_clicked), window);
    gtk_box_pack_start(GTK_BOX(left_vbox), add_file_button, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled, 256, 180);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(left_vbox), scrolled, TRUE, TRUE, 0);

    file_list = gtk_list_box_new();
    g_signal_connect(file_list, "button-press-event", G_CALLBACK(on_creation_listbox_right_click), NULL);
    gtk_container_add(GTK_CONTAINER(scrolled), file_list);

    GtkWidget *form_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(top_hbox), form_vbox, TRUE, TRUE, 0);

    // Сетка для полей
    GtkWidget *form_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(form_grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(form_grid), 12);
    gtk_box_pack_start(GTK_BOX(form_vbox), form_grid, FALSE, FALSE, 0);

    // Столб 1
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Имя");
    gtk_widget_set_hexpand(name_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), name_entry, 0, 0, 1, 1);

    GtkWidget *profession_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(profession_entry), "Профессия");
    gtk_widget_set_hexpand(profession_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), profession_entry, 0, 1, 1, 1);

    GtkWidget *phone_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(phone_entry), "Телефон");
    gtk_widget_set_hexpand(phone_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), phone_entry, 0, 2, 1, 1);

    GtkWidget *telegram_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(telegram_entry), "Telegram");
    gtk_widget_set_hexpand(telegram_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), telegram_entry, 0, 3, 1, 1);

    GtkWidget *whatsapp_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(whatsapp_entry), "WhatsApp");
    gtk_widget_set_hexpand(whatsapp_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), whatsapp_entry, 0, 4, 1, 1);


    // Столб 2
    GtkWidget *email_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(email_entry), "Email");
    gtk_widget_set_hexpand(email_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), email_entry, 1, 0, 1, 1);

    GtkWidget *website_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(website_entry), "Сайт");
    gtk_widget_set_hexpand(website_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), website_entry, 1, 1, 1, 1);

    GtkWidget *country_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(country_entry), "Страна");
    gtk_widget_set_hexpand(country_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), country_entry, 1, 2, 1, 1);

    GtkWidget *city_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(city_entry), "Город");
    gtk_widget_set_hexpand(city_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), city_entry, 1, 3, 1, 1);

    GtkWidget *date_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(date_entry), "Дата");
    gtk_widget_set_hexpand(date_entry, TRUE);
    gtk_grid_attach(GTK_GRID(form_grid), date_entry, 1, 4, 1, 1);
   // gtk_editable_set_editable(GTK_EDITABLE(date_entry), FALSE); // поле только для чтения

    // Получение текущей даты
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char date_str[20];
    snprintf(date_str, sizeof(date_str), "%02d/%02d/%04d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);

    // Установка даты в поле
    gtk_entry_set_text(GTK_ENTRY(date_entry), date_str);


    // --- Отдельно: Описание --- //

    GtkWidget *desc_label = gtk_label_new("Описание:");
    gtk_widget_set_halign(desc_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(form_vbox), desc_label, FALSE, FALSE, 0);

    GtkWidget *description_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(description_view), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_vexpand(description_view, TRUE); // Чтобы растягивалось вниз
    gtk_box_pack_start(GTK_BOX(form_vbox), description_view, TRUE, TRUE, 0);


    field_widgets_search.name_entry = name_entry;
    field_widgets_search.profession_entry = profession_entry;
    field_widgets_search.phone_entry = phone_entry;
    field_widgets_search.telegram_entry = telegram_entry;
    field_widgets_search.whatsapp_entry = whatsapp_entry;
    field_widgets_search.email_entry = email_entry;
    field_widgets_search.website_entry = website_entry;
    field_widgets_search.description_view = description_view;
    field_widgets_search.country_entry = country_entry;
    field_widgets_search.city_entry = city_entry;
    field_widgets_search.date_entry = date_entry;

    GtkWidget *save_button = gtk_button_new_with_label("Сохранить");
    gtk_widget_set_hexpand(save_button, TRUE);
    gtk_box_pack_end(GTK_BOX(creation_page), save_button, FALSE, FALSE, 0);
    g_object_set_data(G_OBJECT(save_button), "name_entry", name_entry);
    g_object_set_data(G_OBJECT(save_button), "profession_entry", profession_entry);
    g_object_set_data(G_OBJECT(save_button), "phone_entry", phone_entry);
    g_object_set_data(G_OBJECT(save_button), "telegram_entry", telegram_entry);
    g_object_set_data(G_OBJECT(save_button), "whatsapp_entry", whatsapp_entry);
    g_object_set_data(G_OBJECT(save_button), "website_entry", website_entry);
    g_object_set_data(G_OBJECT(save_button), "email_entry", email_entry);
    g_object_set_data(G_OBJECT(save_button), "country_entry", country_entry);
    g_object_set_data(G_OBJECT(save_button), "city_entry", city_entry);
    g_object_set_data(G_OBJECT(save_button), "date_entry", date_entry);
    g_object_set_data(G_OBJECT(save_button), "description_view", description_view);

    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), window);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), creation_page, gtk_label_new("Создание"));

    // Вторая вкладка — заглушка
    GtkWidget *search_page = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(search_page), 10);

    GtkWidget *left_vbox_search = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(search_page), left_vbox_search, FALSE, TRUE, 0);

    GtkWidget *search_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(search_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(search_scroll, 200, -1);
    gtk_widget_set_vexpand(search_scroll, TRUE);
    gtk_widget_set_hexpand(search_scroll, FALSE);
    gtk_box_pack_start(GTK_BOX(left_vbox_search), search_scroll, TRUE, TRUE, 0);

    search_list = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(search_scroll), search_list);
    gtk_widget_add_events(search_list, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(search_list, "button-press-event", G_CALLBACK(on_listbox_button_press), NULL);
    g_signal_connect(search_list, "button-press-event", G_CALLBACK(on_listbox_right_click), search_list);



    reset_search_button = gtk_button_new_with_label("Сброс");
    gtk_box_pack_start(GTK_BOX(left_vbox_search), reset_search_button, FALSE, FALSE, 0);
    g_signal_connect(reset_search_button, "clicked", G_CALLBACK(on_reset_button_clicked), search_list);

    add_extra_file_button = gtk_button_new_with_label("Добавить файл");
    gtk_box_pack_start(GTK_BOX(left_vbox_search), add_extra_file_button, FALSE, FALSE, 0);
    g_signal_connect(add_extra_file_button, "clicked", G_CALLBACK(on_add_extra_file_clicked), window);



    GDir *dir = g_dir_open("Portfolios", 0, NULL);
    if (dir) {
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            char full_path[600];
            snprintf(full_path, sizeof(full_path), "Portfolios/%s", filename);
    
            if (g_file_test(full_path, G_FILE_TEST_IS_DIR)) {
                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *label = gtk_label_new(filename);
                gtk_label_set_xalign(GTK_LABEL(label), 0.0);  // по левому краю
                gtk_container_add(GTK_CONTAINER(row), label);
                gtk_list_box_insert(GTK_LIST_BOX(search_list), row, -1);
            }
        }
        g_dir_close(dir);
    }

    // Контейнер справа для кнопок и формы
    GtkWidget *right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(search_page), right_vbox, TRUE, TRUE, 0);

    // Сетка для полей поиска
    GtkWidget *search_form_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(search_form_grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(search_form_grid), 12);
    gtk_box_pack_start(GTK_BOX(right_vbox), search_form_grid, FALSE, FALSE, 0);

    // Столб 1
    GtkWidget *search_name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_name_entry), "Имя");
    gtk_widget_set_hexpand(search_name_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_name_entry, 0, 0, 1, 1);

    GtkWidget *search_profession_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_profession_entry), "Профессия");
    gtk_widget_set_hexpand(search_profession_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_profession_entry, 0, 1, 1, 1);

    GtkWidget *search_phone_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_phone_entry), "Телефон");
    gtk_widget_set_hexpand(search_phone_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_phone_entry, 0, 2, 1, 1);

    GtkWidget *search_telegram_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_telegram_entry), "Telegram");
    gtk_widget_set_hexpand(search_telegram_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_telegram_entry, 0, 3, 1, 1);

    GtkWidget *search_whatsapp_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_whatsapp_entry), "WhatsApp");
    gtk_widget_set_hexpand(search_whatsapp_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_whatsapp_entry, 0, 4, 1, 1);

    // Столб 2
    GtkWidget *search_email_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_email_entry), "Email");
    gtk_widget_set_hexpand(search_email_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_email_entry, 1, 0, 1, 1);

    GtkWidget *search_website_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_website_entry), "Сайт");
    gtk_widget_set_hexpand(search_website_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_website_entry, 1, 1, 1, 1);

    GtkWidget *search_country_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_country_entry), "Страна");
    gtk_widget_set_hexpand(search_country_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_country_entry, 1, 2, 1, 1);

    GtkWidget *search_city_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_city_entry), "Город");
    gtk_widget_set_hexpand(search_city_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_city_entry, 1, 3, 1, 1);

    GtkWidget *search_date_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_date_entry), "Дата");
    gtk_widget_set_hexpand(search_date_entry, TRUE);
    gtk_grid_attach(GTK_GRID(search_form_grid), search_date_entry, 1, 4, 1, 1);

    // Можешь добавить другие поля при необходимости, как email/город и т.д.

    // Метка и поле "Описание"
    GtkWidget *search_description_label = gtk_label_new("Описание:");
    gtk_widget_set_halign(search_description_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(right_vbox), search_description_label, FALSE, FALSE, 0);

    GtkWidget *search_description_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(search_description_view), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_vexpand(search_description_view, TRUE); // Растягивать вниз
    gtk_box_pack_start(GTK_BOX(right_vbox), search_description_view, TRUE, TRUE, 0);



    field_widgets.name_entry = search_name_entry;
    field_widgets.profession_entry = search_profession_entry;
    field_widgets.phone_entry = search_phone_entry;
    field_widgets.telegram_entry = search_telegram_entry;
    field_widgets.whatsapp_entry = search_whatsapp_entry;
    field_widgets.email_entry = search_email_entry;
    field_widgets.website_entry = search_website_entry;
    field_widgets.country_entry = search_country_entry;
    field_widgets.city_entry = search_city_entry;
    field_widgets.date_entry = search_date_entry;
    field_widgets.description_view = search_description_view;



    // Контейнер для кнопок
    GtkWidget *button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(right_vbox), button_hbox, FALSE, FALSE, 0);

    back_button = gtk_button_new_with_label("Назад");
    gtk_widget_set_hexpand(back_button, TRUE);
    gtk_box_pack_start(GTK_BOX(button_hbox), back_button, TRUE, TRUE, 0);
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_button_clicked), search_list);

    update_button = gtk_button_new_with_label("Обновить");
    gtk_widget_set_hexpand(update_button, TRUE);
    gtk_box_pack_start(GTK_BOX(button_hbox), update_button, TRUE, TRUE, 0);
    g_signal_connect(update_button, "clicked", G_CALLBACK(on_update_button_clicked), search_list);

    change_button = gtk_button_new_with_label("Изменить");
    gtk_widget_set_hexpand(change_button, TRUE);
    gtk_box_pack_start(GTK_BOX(button_hbox), change_button, TRUE, TRUE, 0);
    g_signal_connect(change_button, "clicked", G_CALLBACK(on_change_button_clicked), NULL);

    select_button = gtk_button_new_with_label("Выбрать");
    gtk_widget_set_hexpand(select_button, TRUE);
    gtk_box_pack_start(GTK_BOX(button_hbox), select_button, TRUE, TRUE, 0);
    g_signal_connect(select_button, "clicked", G_CALLBACK(on_select_button_clicked), search_list);
    g_signal_connect(search_list, "row-selected", G_CALLBACK(on_search_row_selected), NULL);

    open_button = gtk_button_new_with_label("Открыть");
    gtk_widget_set_hexpand(open_button, TRUE);
    gtk_box_pack_start(GTK_BOX(button_hbox), open_button, TRUE, TRUE, 0);
    g_signal_connect(open_button, "clicked", G_CALLBACK(on_open_button_clicked), NULL);



    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), search_page, gtk_label_new("Поиск"));


    gtk_widget_show_all(window);
    gtk_widget_hide(open_button);
    gtk_widget_hide(back_button);
    gtk_widget_hide(change_button);
    gtk_widget_hide(add_extra_file_button);

    gtk_main();
    return 0;
}