/*
	David Steinberg
	Operating Systems
	Project 1, Part 1
	October 14, 2012
*/

/////////////////////////////////////
//		DEFINE / INCLUDES
/////////////////////////////////////

// Need to yield() on Neptune
#define _GLIBCXX_USE_SCHED_YIELD

#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

/////////////////////////////////////
//		GLOBALS
/////////////////////////////////////

// A vector of the words as they come in
// and a counter to know which words have printed

vector<string> words;
size_t counter = 0;

/////////////////////////////////////
//		AUXILIARY FUNCTIONS
/////////////////////////////////////

// Returns true if character matches a vowel (upper or lower case)
bool isVowel(char c) {
	return (
		c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' ||
		c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'
	);
}

/*
	If there are more words to print,
	checks if the current word starts with a vowel.
	If so, it prints the word and increments the counter.
	If not, it yields to allow the consonant function to run.
*/
void vow () {
	while (counter != words.size()) {
		if (isVowel(words[counter][0])) {
			cout << "vow: " << words[counter] << '\n';
			counter++;
		}
		else
			this_thread::yield();
	}
}

/*
	If there are more words to print,
	checks if the current word starts with a vowel.
	If not, it prints the word and increments the counter.
	If so, it yields to allow the vowel function to run.
*/
void cons () {
	while (counter != words.size()) {
		if (!isVowel(words[counter][0])) {
			cout << "cons: " << words[counter] << '\n';
			counter++;
		}
		else
			this_thread::yield();
	}
}

/////////////////////////////////////
//		MAIN
/////////////////////////////////////

int main (int argc, char ** argv) {

	// Argument check
	if (argc != 2) {
		cerr << "Usage : " << argv[0] << " <fileName>\n";
		exit(1);
	}

	// Open file.
	ifstream in;
	in.open(argv[1]); // phrase.txt

	// Read until file end, but stop if a period is found at a word's end.
	string word;
	while ( !in.eof() ) {
		in >> word;
		words.push_back(word);
		if(word[word.length()-1] == '.')
			break;
	}

	// Close file.
	in.close();

	// Create two threads using the vowel and consonant functions, respectively.
	thread	vowThread(vow),
			consThread(cons);

	// Make sure main waits for threads to finish.
	vowThread.join();
	consThread.join();

	return 0;

}

