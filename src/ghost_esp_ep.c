#include "ghost_esp_ep.h"
#include <dialogs/dialogs.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/text_box.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <gui/modules/text_input.h>
#include <storage/storage.h>
#include <toolbox/path.h>

#define GHOST_APP_FOLDER_HTML "/ext/apps_data/ghost/html"

bool ghost_esp_ep_read_html_file(AppState* app, uint8_t** the_html, size_t* html_size) {
    // Initialize output parameters to safe defaults
    *the_html = NULL;
    *html_size = 0;

    // browse for files
    FuriString* predefined_filepath = furi_string_alloc_set_str(GHOST_APP_FOLDER_HTML);
    FuriString* selected_filepath = furi_string_alloc();
    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, ".html", NULL);

    bool success = false;
    File* index_html = NULL;
    Storage* storage = NULL;

    do {
        if(!dialog_file_browser_show(
               app->dialogs, selected_filepath, predefined_filepath, &browser_options)) {
            break; // User cancelled
        }

        storage = furi_record_open(RECORD_STORAGE);
        index_html = storage_file_alloc(storage);
        if(!storage_file_open(
               index_html, furi_string_get_cstr(selected_filepath), FSAM_READ, FSOM_OPEN_EXISTING)) {
            dialog_message_show_storage_error(app->dialogs, "Cannot open file");
            break;
        }

        uint64_t size = storage_file_size(index_html);
        if(size == 0) {
            dialog_message_show_storage_error(app->dialogs, "File is empty");
            break;
        }

        *the_html = malloc(size);
        if(!*the_html) {
            dialog_message_show_storage_error(app->dialogs, "Memory allocation failed");
            break;
        }

        uint8_t* buf_ptr = *the_html;
        size_t read = 0;
        bool read_success = true;
        while(read < size) {
            size_t to_read = size - read;
            if(to_read > UINT16_MAX) to_read = UINT16_MAX;
            uint16_t now_read = storage_file_read(index_html, buf_ptr, (uint16_t)to_read);
            if(now_read == 0) { // Read error
                read_success = false;
                break;
            }
            read += now_read;
            buf_ptr += now_read;
        }

        if(!read_success) {
            free(*the_html);
            *the_html = NULL;
            dialog_message_show_storage_error(app->dialogs, "Error reading file");
            break;
        }

        *html_size = read;
        success = true;
    } while(false);

    // Cleanup
    if(index_html) {
        storage_file_close(index_html);
        storage_file_free(index_html);
    }
    if(storage) {
        furi_record_close(RECORD_STORAGE);
    }
    furi_string_free(selected_filepath);
    furi_string_free(predefined_filepath);

    return success;
}