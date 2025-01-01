#include <stdlib.h>
#include <string.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/Command.h>
#include <Xm/FileSB.h>
#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <jansson.h>

#include <curl/curl.h>

#include "dbg.h"

//int main(int argc, char* argv[]) {
//	XtSetLanguageProc(NULL, NULL, NULL);
//
//	XtAppContext app;
//
//	Widget toplevel = XtVaOpenApplication(
//		&app, "Demos", NULL, 0, &argc, argv, NULL,
//		sessionShellWidgetClass, NULL
//	);
//
//	Widget parent = XmCreateForm(toplevel, "form", NULL, 0);
//	Widget one = XmCreatePushButton(parent, "One", NULL, 0);
//	Widget two = XmCreatePushButton(parent, "Two", NULL, 0);
//	Widget three = XmCreatePushButton(parent, "Three", NULL, 0);
//
//	XtVaSetValues(
//		one,
//		XmNtopAttachment, XmATTACH_FORM,
//		XmNleftAttachment, XmATTACH_FORM,
//		NULL
//	);
//
//	XtVaSetValues(
//		two,
//		XmNleftAttachment, XmATTACH_WIDGET,
//		XmNleftWidget, one,
//		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
//		XmNtopWidget, one,
//		NULL
//	);
//
//	XtVaSetValues(
//		three,
//		XmNtopAttachment, XmATTACH_WIDGET,
//		XmNtopWidget, one,
//		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
//		XmNleftWidget, one,
//		NULL
//	);
//
//	XtManageChild(one);
//	XtManageChild(two);
//	XtManageChild(three);
//	XtManageChild(parent);
//
//	XtRealizeWidget(toplevel);
//	XtAppMainLoop(app);
//}



void file_cb(Widget, XtPointer, XtPointer);
void help_cb(Widget, XtPointer, XtPointer);
void command_cb(Widget widget, XtPointer client_data, XtPointer call_data); 
void hide_fsb(Widget, XtPointer, XtPointer);
json_t* fetch_word(char* word);
void update(json_t*, Widget);

Widget top_level;
Widget label;
Widget text_w;
Widget one;

char word[1024];
char definition_buffer[4096];
char url[1024]; 
char json_buffer[10240];
const char url_template[] = "https://api.dictionaryapi.dev/api/v2/entries/en/%s";


void help_cb(Widget w, XtPointer client_data, XtPointer call_data) {
	printf("help_cb called\n");
}

void hide_fsb(Widget w, XtPointer client_data, XtPointer call_data) {
	XtUnmanageChild(w);
}


void file_cb(Widget widget, XtPointer client_data, XtPointer call_data) {
	log_err("file_cb called?");
}

void command_cb(Widget widget, XtPointer client_data, XtPointer call_data) {
	XmCommandCallbackStruct* cbs = (XmCommandCallbackStruct*) call_data;

	char* cmd = (char*) XmStringUnparse(
		cbs->value, XmFONTLIST_DEFAULT_TAG,
		XmCHARSET_TEXT, XmCHARSET_TEXT, NULL, 0, XmOUTPUT_ALL);
	debug("command_cb called with command = '%s'", cmd);

	Widget label_w = (Widget) client_data;

	XmString temp_s;

	json_t* root = fetch_word(cmd);

	XmString word_s = NULL;

	if (word_s) {
		debug("word_s was not NULL, freeing first");
		XmStringFree(word_s);
	}
	word_s = XmStringCreateLocalized(cmd);
	debug("created motif string for %s", cmd);

	XtVaSetValues(one, XmNlabelString, word_s, NULL);
	debug("set label string to %s", cmd);

	update(root, label_w);

	/*XmTextSetString(text_w, definition_buffer); */
}

XmString string_append(XmString str1, char* str2, Boolean newline) {

	XmString temp_1, temp_2 = NULL;
	XmString to_return;

	XmString xm_str2 = XmStringGenerate(str2, NULL, XmCHARSET_TEXT, NULL);
	XmString separator = XmStringSeparatorCreate();

	if (str1) {
		debug("text was not empty, concat");
		temp_1 = XmStringCopy(str1);
		temp_2 = XmStringConcat(temp_1, xm_str2);
	} else {
		temp_2 = xm_str2;
	}
	if (newline == True) {
		to_return = XmStringConcat(temp_2, separator);
	} else {
		to_return = temp_2;
	}

	return to_return;
}
int asprintf(char ** ret, const char * format, ...) {
	va_list ap;
	int len;
	size_t buflen;

	/* Figure out how long the string needs to be. */
	va_start(ap, format);
	len = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

	/* Did we fail? */
	if (len < 0)
		goto err0;
	buflen = (size_t)(len) + 1;

	/* Allocate memory. */
	if ((*ret = malloc(buflen)) == NULL)
		goto err0;

	/* Actually generate the string. */
	va_start(ap, format);
	len = vsnprintf(*ret, buflen, format, ap);
	va_end(ap);

	/* Did we fail? */
	if (len < 0)
		goto err1;

	/* Success! */
	return (len);

err1:
	free(*ret);
err0:
	/* Failure! */
	return (-1);
}

void wrap_text(char *line_start, int width) {
  char *last_space = 0;
  char *p;

  for (p = line_start; *p; p++) {
    if (*p == '\n') {
      line_start = p + 1;
    }

    if (*p == ' ') {
      last_space = p;
    }

    if (p - line_start > width && last_space) {
      *last_space = '\n';
      line_start = last_space + 1;
      last_space = 0;
    }
  }
}

void update(json_t* root, Widget label_w) {
	if (root == NULL) {
		log_err("json data was NULL");
		return;
	}

	XmString text = NULL;

	/* sew ([0]['word'], XmString, bold)
	 *   
	 *   verb: To use a needle to pass thread repeatedly through (pieces of fabric) in order to join them together. ([0]["meanings"][0]['definitions'][0]['definition'], char*)
	 *   verb: To use a needle to pass thread repeatedly through pieces of fabric in order to join them together. ([0]["meanings"][0]['definitions'][1]['definition'], char*)
	 *   ...
	 * sew ([1]['word'], XmString, bold)
	 *   verb:  To drain the water from. ([1]["meanings"][0]['definitions'][0]['definition'])
	 */

	int num_words = json_array_size(root);

	char* buffer = NULL;	
	for (int word_index = 0; word_index < num_words; word_index++) {
		json_t* word = json_array_get(root, word_index);
		debug("label_w: %p", label_w);
		text = string_append(text, (String)json_string_value(json_object_get(word, "word")), True);

		debug("word_index %d", word_index);
		json_t* meanings = json_object_get(word, "meanings");
		int num_parts_of_speech = json_array_size(meanings);
		for (
			int parts_of_speech_index = 0;
			parts_of_speech_index < num_parts_of_speech;
			parts_of_speech_index++
		) {
			json_t* part_of_speech = json_array_get(meanings, parts_of_speech_index);
			char* part_of_speech_s = (String)json_string_value(json_object_get(part_of_speech, "partOfSpeech"));
			debug("partOfSpeech: %s", part_of_speech_s);
			json_t* definitions = json_object_get(part_of_speech, "definitions");
			for (int definition_index = 0; definition_index < json_array_size(definitions); definition_index++) {
				json_t* definition = json_array_get(definitions, definition_index);
				char* definition_s = (String)json_string_value(json_object_get(definition, "definition"));
				debug("  definition %d: %s", definition_index, definition_s);
				asprintf(&buffer, "  %s: %s", part_of_speech_s, definition_s);
				wrap_text(buffer, 72);
				text = string_append(text, buffer, True);

			}
		}
	}

	XtVaSetValues(label_w,
		XmNlabelString, text,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);
}

struct MemoryStruct {
	char* memory;
	size_t size;
};

static size_t write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct*) userp;
	debug("realsize: %zu, size: %zu, nmemb: %zu", realsize, size, nmemb);
	char* ptr = realloc(mem->memory, mem->size + realsize +1 );
	if(!ptr) {
		log_err("out of memory (realloc returned NULL)");
		return 0;
	}
	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

json_t* fetch_word(char* word) {
	int result;
	result = sprintf(url, url_template, word);

	struct MemoryStruct chunk = {.memory = malloc(1), .size = 0};

	curl_global_init(CURL_GLOBAL_ALL);
	CURL* curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	debug("attempting to fetch from '%s'", url);
	CURLcode res = curl_easy_perform(curl_handle);

	if(res += CURLE_OK) {
		log_err("curl_easy_perform() failed: %s", curl_easy_strerror(res));
	} else {
		debug("%lu bytes retreived", (unsigned long) chunk.size);
	}

	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();


	json_error_t error;
	json_t* root = json_loads(chunk.memory, 0, &error);


	return root;
}



int main(int argc, char* argv[]) {
	Widget main_w;
	XtAppContext app_context;

	Arg al[10];
	Cardinal ac = 0;

	XtSetLanguageProc(NULL, NULL, NULL);

	top_level = XtVaOpenApplication(&app_context, "App-Class", NULL, 0, &argc,
			argv, NULL, sessionShellWidgetClass, NULL);

	/* Create main window */
	ac = 0;
	XtSetArg(al[ac], XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE); ac++;
	XtSetArg(al[ac], XmNwidth, 450); ac++;
	main_w = XmCreateMainWindow(top_level, "main_window", al, ac);

	/* create menubar */
	XmString file = XmStringCreateLocalized("File");
	Widget menubar = XmVaCreateSimpleMenuBar(
		main_w, "menubar",
		XmVaCASCADEBUTTON, file, 'F',
		NULL);
	XmStringFree(file);

	XmString quit = XmStringCreateLocalized("Quit");
	Widget file_menu = XmVaCreateSimplePulldownMenu(
		menubar, "file_menu", 0, (void(*)())exit,
		XmVaPUSHBUTTON, quit, 'Q', 
		NULL);
	XmStringFree(quit);
	XtManageChild(menubar);

	Widget workform = XmCreateForm(main_w, "WorkForm", NULL, 0);
	//Widget one = XmCreatePushButton(workform, "One", NULL, 0);
	one = XtVaCreateManagedWidget(
			"one_label", xmLabelWidgetClass, workform, 
			XtVaTypedArg, XmNlabelString, XmRString,
			"FUUUUCK", 8,
		       NULL);	

	XtVaSetValues(
		one,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNheight, 700,
		NULL
	);

	XtManageChild(one);
	XtManageChild(workform);

	/* command area */
	XmString command = XmStringCreateLocalized("Word:");
	ac = 0;
	XtSetArg(al[ac], XmNpromptString, command); ac++;
	Widget command_w = XmCreateCommand(main_w, "command_w", al, ac);

	XmStringFree(command);
	XtAddCallback(command_w, XmNcommandEnteredCallback, command_cb, one);
	
	debug("added callback with client_data: %p", one);
	
//	XtVaSetValues(command_w, 
//			XmNmenuBar, menubar,
//			XmNcommandWindow, command_w,
//			XmNworkWindow, XtParent(text_w), 
//			NULL);
	XtVaSetValues(command_w, 
			XmNmenuBar, menubar,
			XmNcommandWindow, command_w,
			XmNworkWindow, workform, 
			NULL);
	XtManageChild(command_w);
	XtManageChild(main_w);

	XtRealizeWidget(top_level);
	XtAppMainLoop(app_context);
}

