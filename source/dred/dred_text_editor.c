// Copyright (C) 2016 David Reid. See included LICENSE file.

dred_textview* dred_text_editor__get_textview(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return NULL;
    }

    return pTextEditor->pTextView;
}

void dred_text_editor__register_style(dred_text_editor* pTextEditor, dred_text_style* pStyle)
{
    dred_gui_font_metrics fontMetrics;
    dred_gui_get_font_metrics(pStyle->pFont, &fontMetrics);

    drte_font_metrics drteFontMetrics = drte_font_metrics_create(fontMetrics.ascent, fontMetrics.descent, fontMetrics.lineHeight, fontMetrics.spaceWidth);
    drte_engine_register_style_token(dred_textview_get_engine(dred_text_editor__get_textview(pTextEditor)), (drte_style_token)pStyle, drteFontMetrics);
}


void dred_text_editor__on_size(dred_control* pControl, float newWidth, float newHeight)
{
    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(pControl);
    assert(pTextEditor != NULL);

    dred_textview* pTextView = dred_text_editor__get_textview(pTextEditor);
    if (pTextView == NULL) {
        return;
    }

    // The text box should take up the entire area of the editor.
    dred_control_set_size(DRED_CONTROL(pTextView), newWidth, newHeight);
}

void dred_text_editor__on_capture_keyboard(dred_control* pControl, dred_control* pPrevCapturedControl)
{
    (void)pPrevCapturedControl;

    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(pControl);
    assert(pTextEditor != NULL);
    
    // When a text editor receives keyboard focus it should be routed down to the text box control.
    dred_textview* pTextView = dred_text_editor__get_textview(pTextEditor);
    if (pTextView == NULL) {
        return;
    }

    dred_capture_keyboard(dred_control_get_context(pControl), DRED_CONTROL(pTextView));
}

void dred_text_editor_textview__on_key_down(dred_control* pControl, dred_key key, int stateFlags)
{
    dred_textview* pTextView = DRED_TEXTVIEW(pControl);
    assert(pTextView != NULL);

    if (key == DRED_GUI_ESCAPE) {
        dred_focus_command_bar(dred_control_get_context(pControl));
    } else {
        dred_textview_on_key_down(DRED_CONTROL(pTextView), key, stateFlags);
    }
}

void dred_text_editor_textview__on_mouse_wheel(dred_control* pControl, int delta, int mousePosX, int mousePosY, int stateFlags)
{
    dred_textview* pTextView = DRED_TEXTVIEW(pControl);
    assert(pTextView != NULL);

    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(dred_control_get_parent(pControl));
    if (pTextEditor == NULL) {
        return;
    }

    dred_context* pDred = dred_control_get_context(pControl);
    assert(pDred != NULL);

    if (stateFlags & DRED_GUI_KEY_STATE_CTRL_DOWN) {
        // When setting the scale, we actually do it application-wide, not just local to the current text editor.
        float oldTextScale = dred_get_text_editor_scale(pDred);
        float newTextScale;
        if (delta > 0) {
            newTextScale = oldTextScale * (1.0f + ( delta * 0.1f));
        } else {
            newTextScale = oldTextScale / (1.0f + (-delta * 0.1f));
        }

        // Always make sure the 100% scale is selectable.
        if ((newTextScale < 1 && oldTextScale > 1) || (newTextScale > 1 && oldTextScale < 1)) {
            newTextScale = 1;
        }


        dred_set_text_editor_scale(pDred, newTextScale);
    } else {
        dred_textview_on_mouse_wheel(DRED_CONTROL(pTextView), delta, mousePosX, mousePosY, stateFlags);
    }
}

void dred_text_editor_textview__on_mouse_button_up(dred_control* pControl, int mouseButton, int mousePosX, int mousePosY, int stateFlags)
{
    dred_textview* pTextView = DRED_TEXTVIEW(pControl);
    assert(pTextView != NULL);

    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(dred_control_get_parent(pControl));
    if (pTextEditor == NULL) {
        return;
    }

    dred_context* pDred = dred_control_get_context(pControl);
    assert(pDred != NULL);

    if (mouseButton == DRED_GUI_MOUSE_BUTTON_RIGHT) {
        dred_control_show_popup_menu(pControl, pDred->menuLibrary.pPopupMenu_TextEditor, mousePosX, mousePosY);
    } else {
        dred_textview_on_mouse_button_up(DRED_CONTROL(pTextView), mouseButton, mousePosX, mousePosY, stateFlags);
    }
}

void dred_text_editor_textview__on_cursor_move(dred_textview* pTextView)
{
    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(dred_control_get_parent(DRED_CONTROL(pTextView)));
    assert(pTextEditor != NULL);

    dred_update_info_bar(dred_control_get_context(DRED_CONTROL(pTextEditor)), DRED_CONTROL(pTextEditor));
}

void dred_text_editor_textview__on_capture_keyboard(dred_control* pControl, dred_control* pPrevCapturedControl)
{
    dred_textview* pTextView = DRED_TEXTVIEW(pControl);
    assert(pTextView != NULL);

    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(dred_control_get_parent(pControl));
    if (pTextEditor == NULL) {
        return;
    }

    dred_context* pDred = dred_control_get_context(pControl);
    assert(pDred != NULL);

    if (pDred->config.enableAutoReload) {
        dred_editor_check_if_dirty_and_reload(DRED_EDITOR(pTextEditor));
    }

    // Fall through to the text boxes normal capture_keyboard event handler...
    dred_textview_on_capture_keyboard(DRED_CONTROL(pTextView), pPrevCapturedControl);
}


void dred_text_editor_engine__on_text_changed(drte_engine* pTextEngine)
{
    dred_text_editor* pTextEditor = (dred_text_editor*)pTextEngine->pUserData;
    assert(pTextEditor != NULL);
    
    // TODO: Correctly handle multiple views.
    dred_textview__on_text_changed(pTextEditor->pTextView);
}

void dred_text_editor_engine__on_undo_point_changed(drte_engine* pTextEngine, unsigned int iUndoPoint)
{
    //dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(dred_control_get_parent(DRED_CONTROL(pTextView)));
    dred_text_editor* pTextEditor = (dred_text_editor*)pTextEngine->pUserData;
    if (pTextEditor == NULL) {
        return;
    }

    if (iUndoPoint == pTextEditor->iBaseUndoPoint) {
        dred_editor_unmark_as_modified(DRED_EDITOR(pTextEditor));
    } else {
        dred_editor_mark_as_modified(DRED_EDITOR(pTextEditor));
    }
}

size_t dred_text_editor_engine__on_get_undo_state(drte_engine* pTextEngine, void* pDataOut)
{
    dred_text_editor* pTextEditor = (dred_text_editor*)pTextEngine->pUserData;
    assert(pTextEditor != NULL);

    // TODO: Correctly handle multiple views. Will need to gather all of the data into a single buffer.
    return dred_textview__on_get_undo_state(pTextEditor->pTextView, pDataOut);
}

void dred_text_editor_engine__on_apply_undo_state(drte_engine* pTextEngine, size_t dataSize, const void* pData)
{
    dred_text_editor* pTextEditor = (dred_text_editor*)pTextEngine->pUserData;
    assert(pTextEditor != NULL);

    // TODO: Correctly handle multiple views.
    dred_textview__on_apply_undo_state(pTextEditor->pTextView, dataSize, pData);
}

void dred_text_editor_engine__on_undo_stack_trimmed(drte_engine* pTextEngine)
{
    dred_text_editor* pTextEditor = (dred_text_editor*)pTextEngine->pUserData;
    assert(pTextEditor != NULL);

    if (drte_engine_get_undo_points_remaining_count(pTextEngine) < pTextEditor->iBaseUndoPoint) {
        pTextEditor->iBaseUndoPoint = (unsigned int)-1;
    }
}

dr_bool32 dred_text_editor__on_save(dred_editor* pEditor, dred_file file, const char* filePath)
{
    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(pEditor);
    assert(pTextEditor != NULL);

    dred_textview* pTextView = dred_text_editor__get_textview(pTextEditor);
    if (pTextView == NULL) {
        return DR_FALSE;
    }

    size_t textLength = dred_textview_get_text(pTextView, NULL, 0);
    char* text = (char*)malloc(textLength + 1);
    if (text == NULL) {
        return DR_FALSE;
    }
    dred_textview_get_text(pTextView, text, textLength+1);

    dr_bool32 result = dred_file_write_string(file, text);
    free(text);

    // After saving we need to update the base undo point and unmark the file as modified.
    if (result) {
        pTextEditor->iBaseUndoPoint = dred_textview_get_undo_points_remaining_count(pTextView);

        // Syntax highlighting needs to be updated based on the file extension.
        dred_text_editor_set_highlighter(pTextEditor, dred_get_language_by_file_path(dred_control_get_context(DRED_CONTROL(pTextEditor)), filePath));
    }

    return result;
}

dr_bool32 dred_text_editor__on_reload(dred_editor* pEditor)
{
    dred_text_editor* pTextEditor = DRED_TEXT_EDITOR(pEditor);
    assert(pTextEditor != NULL);

    dred_textview* pTextView = dred_text_editor__get_textview(pTextEditor);
    if (pTextView == NULL) {
        return DR_FALSE;
    }

    char* pFileData = dr_open_and_read_text_file(dred_editor_get_file_path(DRED_EDITOR(pTextEditor)), NULL);
    if (pFileData == NULL) {
        return DR_FALSE;
    }

    dred_textview_set_text(pTextEditor->pTextView, pFileData);
    dr_free_file_data(pFileData);

    // After reloading we need to update the base undo point and unmark the file as modified.
    pTextEditor->iBaseUndoPoint = dred_textview_get_undo_points_remaining_count(pTextView);
    dred_editor_unmark_as_modified(DRED_EDITOR(pTextEditor));

    return DR_TRUE;
}

dred_text_editor* dred_text_editor_create(dred_context* pDred, dred_control* pParent, float sizeX, float sizeY, const char* filePathAbsolute)
{
    dred_text_editor* pTextEditor = (dred_text_editor*)calloc(1, sizeof(*pTextEditor));
    if (pTextEditor == NULL) {
        return NULL;
    }

    if (!dred_editor_init(DRED_EDITOR(pTextEditor), pDred, pParent, DRED_CONTROL_TYPE_TEXT_EDITOR, sizeX, sizeY, filePathAbsolute)) {
        free(pTextEditor);
        return NULL;
    }

    if (!drte_engine_init(&pTextEditor->engine, pTextEditor)) {
        free(pTextEditor);
        return NULL;
    }

    drte_engine_set_on_text_changed(&pTextEditor->engine, dred_text_editor_engine__on_text_changed);
    drte_engine_set_on_undo_point_changed(&pTextEditor->engine, dred_text_editor_engine__on_undo_point_changed);
    pTextEditor->engine.onUndoStackTrimmed = dred_text_editor_engine__on_undo_stack_trimmed;
    pTextEditor->engine.onGetUndoState = dred_text_editor_engine__on_get_undo_state;
    pTextEditor->engine.onApplyUndoState = dred_text_editor_engine__on_apply_undo_state;


    pTextEditor->pTextView = &pTextEditor->textView;
    if (!dred_textview_init(pTextEditor->pTextView, pDred, DRED_CONTROL(pTextEditor), &pTextEditor->engine)) {
        dred_editor_uninit(DRED_EDITOR(pTextEditor));
        free(pTextEditor);
        return NULL;
    }

    dred_control_set_size(DRED_CONTROL(pTextEditor->pTextView), sizeX, sizeY);

    pTextEditor->textScale = 1;
    dred_text_editor_set_highlighter(pTextEditor, dred_get_language_by_file_path(pDred, filePathAbsolute));

    if (filePathAbsolute != NULL && filePathAbsolute[0] != '\0') {
        char* pFileData = dr_open_and_read_text_file(filePathAbsolute, NULL);
        if (pFileData == NULL) {
            dred_textview_uninit(pTextEditor->pTextView);
            drte_engine_uninit(&pTextEditor->engine);
            dred_editor_uninit(DRED_EDITOR(pTextEditor));
            free(pTextEditor);
            return NULL;
        }

        dred_textview_set_text(pTextEditor->pTextView, pFileData);
        dred_textview_clear_undo_stack(pTextEditor->pTextView);
        dr_free_file_data(pFileData);
    }


    // Events.
    dred_control_set_on_size(DRED_CONTROL(pTextEditor), dred_text_editor__on_size);
    dred_control_set_on_capture_keyboard(DRED_CONTROL(pTextEditor), dred_text_editor__on_capture_keyboard);
    dred_editor_set_on_save(DRED_EDITOR(pTextEditor), dred_text_editor__on_save);
    dred_editor_set_on_reload(DRED_EDITOR(pTextEditor), dred_text_editor__on_reload);
    dred_control_set_on_mouse_button_up(DRED_CONTROL(pTextEditor->pTextView), dred_text_editor_textview__on_mouse_button_up);
    dred_control_set_on_mouse_wheel(DRED_CONTROL(pTextEditor->pTextView), dred_text_editor_textview__on_mouse_wheel);
    dred_control_set_on_key_down(DRED_CONTROL(pTextEditor->pTextView), dred_text_editor_textview__on_key_down);
    dred_control_set_on_capture_keyboard(DRED_CONTROL(pTextEditor->pTextView), dred_text_editor_textview__on_capture_keyboard);
    dred_textview_set_on_cursor_move(pTextEditor->pTextView, dred_text_editor_textview__on_cursor_move);
    //dred_textview_set_on_undo_point_changed(pTextEditor->pTextView, dred_text_editor_textview__on_undo_point_changed);

    // Initialize the styling.
    dred_text_editor_refresh_styling(pTextEditor);

    // Word wrap.
    if (pDred->config.textEditorEnableWordWrap) {
        dred_text_editor_enable_word_wrap(pTextEditor);
    }
    
    
    return pTextEditor;
}

void dred_text_editor_delete(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_uninit(pTextEditor->pTextView);
    drte_engine_uninit(&pTextEditor->engine);

    dred_editor_uninit(DRED_EDITOR(pTextEditor));
    free(pTextEditor);
}


void dred_text_editor_set_text(dred_text_editor* pTextEditor, const char* text)
{
    if (pTextEditor == NULL) {
        return;
    }

    if (text == NULL) {
        text = "";
    }

    dred_textview_set_text(dred_text_editor_get_focused_view(pTextEditor), text);
}

size_t dred_text_editor_get_text(dred_text_editor* pTextEditor, char* pTextOut, size_t textOutSize)
{
    if (pTextEditor == NULL) {
        return 0;
    }

    return dred_textview_get_text(pTextEditor->pTextView, pTextOut, textOutSize);
}

size_t dred_text_editor_get_selected_text(dred_text_editor* pTextEditor, char* pTextOut, size_t textOutSize)
{
    if (pTextEditor == NULL) {
        return 0;
    }

    return dred_textview_get_selected_text(dred_text_editor_get_focused_view(pTextEditor), pTextOut, textOutSize);
}


dred_textview* dred_text_editor_get_focused_view(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return NULL;
    }

    return pTextEditor->pTextView;
}


void dred_text_editor_enable_word_wrap(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_enable_word_wrap(pTextEditor->pTextView);
}

void dred_text_editor_disable_word_wrap(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_disable_word_wrap(pTextEditor->pTextView);
}

dr_bool32 dred_text_editor_is_word_wrap_enabled(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return DR_FALSE;
    }

    return dred_textview_is_word_wrap_enabled(pTextEditor->pTextView);
}


void dred_text_editor_enable_drag_and_drop(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_enable_drag_and_drop(pTextEditor->pTextView);
}

void dred_text_editor_disable_drag_and_drop(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_disable_drag_and_drop(pTextEditor->pTextView);
}

dr_bool32 dred_text_editor_is_drag_and_drop_enabled(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return DR_FALSE;
    }

    return dred_textview_is_drag_and_drop_enabled(pTextEditor->pTextView);
}


dr_bool32 dred_text_editor_insert_text_at_cursors(dred_text_editor* pTextEditor, const char* text)
{
    if (pTextEditor == NULL) {
        return DR_FALSE;
    }

    return dred_textview_insert_text_at_cursors(pTextEditor->pTextView, text);
}


void dred_text_editor_refresh_styling(dred_text_editor* pTextEditor)
{
    assert(pTextEditor != NULL);

    dred_context* pDred = dred_control_get_context(DRED_CONTROL(pTextEditor));
    assert(pDred != NULL);

    dred_control_begin_dirty(DRED_CONTROL(pTextEditor));
    {
        // Highlighting.
        pTextEditor->highlighter.styles.common.comment.pFont = dred_font_acquire_subfont(pDred->config.pTextEditorFont, pDred->config.textEditorScale);
        pTextEditor->highlighter.styles.common.comment.bgColor = pDred->config.textEditorBGColor;
        pTextEditor->highlighter.styles.common.comment.fgColor = pDred->config.cppCommentTextColor;
        dred_text_editor__register_style(pTextEditor, &pTextEditor->highlighter.styles.common.comment);

        pTextEditor->highlighter.styles.common.string.pFont = dred_font_acquire_subfont(pDred->config.pTextEditorFont, pDred->config.textEditorScale);
        pTextEditor->highlighter.styles.common.string.bgColor = pDred->config.textEditorBGColor;
        pTextEditor->highlighter.styles.common.string.fgColor = pDred->config.cppStringTextColor;
        dred_text_editor__register_style(pTextEditor, &pTextEditor->highlighter.styles.common.string);
        
        pTextEditor->highlighter.styles.common.keyword.pFont = dred_font_acquire_subfont(pDred->config.pTextEditorFont, pDred->config.textEditorScale);
        pTextEditor->highlighter.styles.common.keyword.bgColor = pDred->config.textEditorBGColor;
        pTextEditor->highlighter.styles.common.keyword.fgColor = pDred->config.cppKeywordTextColor;
        dred_text_editor__register_style(pTextEditor, &pTextEditor->highlighter.styles.common.keyword);


        dred_textview_set_font(pTextEditor->pTextView, dred_font_acquire_subfont(pDred->config.pTextEditorFont, pDred->uiScale));    // TODO: <-- This font needs to be unacquired.
        dred_textview_set_text_color(pTextEditor->pTextView, pDred->config.textEditorTextColor);
        dred_textview_set_cursor_color(pTextEditor->pTextView, pDred->config.textEditorCursorColor);
        dred_textview_set_background_color(pTextEditor->pTextView, pDred->config.textEditorBGColor);
        dred_textview_set_selection_background_color(pTextEditor->pTextView, pDred->config.textEditorSelectionBGColor);
        dred_textview_set_active_line_background_color(pTextEditor->pTextView, pDred->config.textEditorActiveLineColor);
        dred_textview_set_padding(pTextEditor->pTextView, 0);
        dred_textview_set_line_numbers_color(pTextEditor->pTextView, pDred->config.textEditorLineNumbersColor);
        dred_textview_set_line_numbers_background_color(pTextEditor->pTextView, pDred->config.textEditorLineNumbersBGColor);
        dred_textview_set_line_numbers_padding(pTextEditor->pTextView, pDred->config.textEditorLineNumbersPadding);
    
        dred_textview_set_scrollbar_track_color(pTextEditor->pTextView, pDred->config.textEditorSBTrackColor);
        dred_textview_set_scrollbar_thumb_color(pTextEditor->pTextView, pDred->config.textEditorSBThumbColor);
        dred_textview_set_scrollbar_thumb_color_hovered(pTextEditor->pTextView, pDred->config.textEditorSBThumbColorHovered);
        dred_textview_set_scrollbar_thumb_color_pressed(pTextEditor->pTextView, pDred->config.textEditorSBThumbColorPressed);
        dred_textview_set_scrollbar_size(pTextEditor->pTextView, pDred->config.textEditorSBSize * pDred->uiScale);
        if (pDred->config.textEditorShowScrollbarHorz) {
            dred_textview_enable_horizontal_scrollbar(pTextEditor->pTextView);
        } else {
            dred_textview_disable_horizontal_scrollbar(pTextEditor->pTextView);
        }
        if (pDred->config.textEditorShowScrollbarVert) {
            dred_textview_enable_vertical_scrollbar(pTextEditor->pTextView);
        } else {
            dred_textview_disable_vertical_scrollbar(pTextEditor->pTextView);
        }
        if (pDred->config.textEditorEnableExcessScrolling) {
            dred_textview_enable_excess_scrolling(pTextEditor->pTextView);
        } else {
            dred_textview_disable_excess_scrolling(pTextEditor->pTextView);
        }


        dred_textview_set_tab_size_in_spaces(pTextEditor->pTextView, pDred->config.textEditorTabSizeInSpaces);
        if (pDred->config.textEditorTabsToSpacesEnabled) {
            dred_textview_enable_tabs_to_spaces(pTextEditor->pTextView);
        } else {
            dred_textview_disable_tabs_to_spaces(pTextEditor->pTextView);
        }

        if (pDred->config.textEditorShowLineNumbers) {
            dred_text_editor_show_line_numbers(pTextEditor);
        } else {
            dred_text_editor_hide_line_numbers(pTextEditor);
        }

        dred_text_editor_set_text_scale(pTextEditor, pDred->config.textEditorScale);
    }
    dred_control_end_dirty(DRED_CONTROL(pTextEditor));
}

void dred_text_editor_set_highlighter(dred_text_editor* pTextEditor, const char* lang)
{
    if (pTextEditor == NULL) {
        return;
    }

    drte_engine* pEngine = dred_textview_get_engine(dred_text_editor__get_textview(pTextEditor));
    assert(pEngine != NULL);

    dred_context* pDred = dred_control_get_context(DRED_CONTROL(pTextEditor));
    assert(pDred != NULL);

    if (lang == NULL) {
        drte_engine_set_highlighter(pEngine, NULL, NULL);
    } else {
        // Unfortunately highlighting is not quite ready for prime time.
        drte_engine_set_highlighter(pEngine, NULL, NULL);
        (void)pDred;
        (void)pTextEditor;
#if 0
        if (strcmp(lang, "c") == 0) {
            dred_highlighter_init(&pTextEditor->highlighter, pDred, dred_textview_get_engine(pTextEditor->pTextView), g_KeywordsC, sizeof(g_KeywordsC) / sizeof(g_KeywordsC[0]));
            drte_engine_set_highlighter(dred_textview_get_engine(pTextEditor->pTextView), pTextEditor->highlighter.onNextHighlight, &pTextEditor->highlighter);
        }
#endif
    }
}


void dred_text_editor_set_font(dred_text_editor* pTextEditor, dred_font* pFont)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_set_font(pTextEditor->pTextView, dred_font_acquire_subfont(pFont, pFont->pDred->uiScale));
}


void dred_text_editor_show_line_numbers(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_show_line_numbers(pTextEditor->pTextView);
}

void dred_text_editor_hide_line_numbers(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_hide_line_numbers(pTextEditor->pTextView);
}


size_t dred_text_editor_get_cursor_line(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return 0;
    }

    return dred_textview_get_cursor_line(pTextEditor->pTextView);
}

size_t dred_text_editor_get_cursor_column(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return 0;
    }

    return dred_textview_get_cursor_column(pTextEditor->pTextView);
}


void dred_text_editor_goto_ratio(dred_text_editor* pTextEditor, size_t ratio)
{
    if (pTextEditor == NULL) {
        return;
    }

    if (ratio > 100) {
        ratio = 100;
    }

    dred_text_editor_goto_line(pTextEditor, (size_t)(roundf(dred_textview_get_line_count(pTextEditor->pTextView) * (ratio/100.0f))));
}

void dred_text_editor_goto_line(dred_text_editor* pTextEditor, size_t lineNumber)
{
    if (pTextEditor == NULL) {
        return;
    }

    if (lineNumber == 0) {
        lineNumber = 1;
    }
    if (lineNumber > dred_textview_get_line_count(pTextEditor->pTextView)) {
        lineNumber = dred_textview_get_line_count(pTextEditor->pTextView);
    }

    dred_textview_deselect_all(pTextEditor->pTextView);

    dr_bool32 centerView = !dred_textview_is_unwrapped_line_in_view(pTextEditor->pTextView, lineNumber - 1);
    dred_textview_move_cursor_to_start_of_unwrapped_line_by_index(pTextEditor->pTextView, lineNumber - 1);

    if (centerView) {
        dred_textview_center_on_cursor(pTextEditor->pTextView);
    }
}


void dred_text_editor_deselect_all_in_focused_view(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_deselect_all(pTextEditor->pTextView);
}


dr_bool32 dred_text_editor_find_and_select_next(dred_text_editor* pTextEditor, const char* text)
{
    if (pTextEditor == NULL) {
        return DR_FALSE;
    }

    return dred_textview_find_and_select_next(pTextEditor->pTextView, text);
}

dr_bool32 dred_text_editor_find_and_replace_next(dred_text_editor* pTextEditor, const char* text, const char* replacement)
{
    if (pTextEditor == NULL) {
        return DR_FALSE;
    }

    return dred_textview_find_and_replace_next(pTextEditor->pTextView, text, replacement);
}

dr_bool32 dred_text_editor_find_and_replace_all(dred_text_editor* pTextEditor, const char* text, const char* replacement)
{
    if (pTextEditor == NULL) {
        return DR_FALSE;
    }

    dr_bool32 result = DR_FALSE;
    dred_control_begin_dirty(DRED_CONTROL(pTextEditor));
    {
        result = dred_textview_find_and_replace_all(pTextEditor->pTextView, text, replacement);
    }
    dred_control_end_dirty(DRED_CONTROL(pTextEditor));
    return result;
}


void dred_text_editor_set_text_scale(dred_text_editor* pTextEditor, float textScale)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_context* pDred = dred_control_get_context(DRED_CONTROL(pTextEditor));
    assert(pDred != NULL);

    pTextEditor->textScale = dr_clamp(textScale, 0.1f, 4.0f);
    dred_textview_set_line_numbers_width(pTextEditor->pTextView, (48 + pDred->config.textEditorLineNumbersPadding) * pDred->uiScale * pTextEditor->textScale);
    dred_textview_set_line_numbers_padding(pTextEditor->pTextView, pDred->config.textEditorLineNumbersPadding * pDred->uiScale * pTextEditor->textScale);
    dred_textview_set_font(pTextEditor->pTextView, dred_font_acquire_subfont(pDred->config.pTextEditorFont, pDred->uiScale * pTextEditor->textScale));    // TODO: <-- This font needs to be unacquired.
    dred_textview_set_cursor_width(pTextEditor->pTextView, pDred->config.textEditorCursorWidth * pDred->uiScale * pTextEditor->textScale);
}

void dred_text_editor_unindent_selected_blocks(dred_text_editor* pTextEditor)
{
    if (pTextEditor == NULL) {
        return;
    }

    dred_textview_unindent_selected_blocks(pTextEditor->pTextView);
}