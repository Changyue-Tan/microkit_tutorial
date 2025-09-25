#include <microkit.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "wordle.h"

#define CHANNEL_TO_CLIENT 1
#define CHANNEL_TO_VMM    2

#define WORDLE_WORD_SIZE 5

/*
 * Here we initialise the word to "hello", but later in the tutorial
 * we will actually randomise the word the user is guessing.
 */
char word[WORD_LENGTH] = {'h', 'e', 'l', 'l', 'o'};

bool is_character_in_word(char *word, int ch) {
	for (int i = 0; i < WORD_LENGTH; i++) {
		if (word[i] == ch) {
			return true;
		}
	}

	return false;
}

enum character_state char_to_state(int ch, char *word, uint64_t index) {
	if (ch == word[index]) {
		return CORRECT_PLACEMENT;
	} else if (is_character_in_word(word, ch)) {
		return INCORRECT_PLACEMENT;
	} else {
		return INCORRECT;
	}
}

void init(void) {
	microkit_dbg_puts("WORDLE SERVER: starting\n");
}

microkit_msginfo protected(microkit_channel channel, microkit_msginfo msginfo) {
	microkit_msginfo message_structure_to_be_returned =
	    microkit_msginfo_new(0, 0);

	if (channel == CHANNEL_TO_CLIENT) {
		int entered_word[WORDLE_WORD_SIZE];
		int entered_word_states[WORDLE_WORD_SIZE];
		// get the word passed from the message registers
		for (int i = 0; i < WORDLE_WORD_SIZE; i++) {
			entered_word[i] = microkit_mr_get(i);
		}
		// check if the word passed from the messages registers are correct
		for (int i = 0; i < WORDLE_WORD_SIZE; i++) {
			entered_word_states[i] = char_to_state(entered_word[i], word, i);
		}
		// return the states back to the caller of this protection procedure
		message_structure_to_be_returned =
		    microkit_msginfo_new(0, WORDLE_WORD_SIZE);
		for (int i = 0; i < WORDLE_WORD_SIZE; i++) {
			microkit_mr_set(i, entered_word_states[i]);
		}
	} else if (channel == CHANNEL_TO_VMM) {
		// update the word array to the random new word
		for (int i = 0; i < WORDLE_WORD_SIZE; i++) {
			word[i] = microkit_mr_get(i);
		}
	}
	return message_structure_to_be_returned;
}

void notified(microkit_channel channel) {
	switch (channel) {
		case CHANNEL_TO_CLIENT:
			microkit_dbg_puts("WORDLE SERVER: Received notification from client!\n");
			break;
	}
}
