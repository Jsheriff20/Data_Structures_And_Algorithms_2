#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <limits>
#include <stdio.h>
#include <queue>
#include <mutex>
#include <thread>

#define NO_OF_CHARS 256 //sets the amount of possible characters (from the ASCII code of an 8 bit program 0-255 (256 possibilities)) 


using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::lock_guard;
typedef std::chrono::steady_clock the_clock;
using namespace std;


//global variables
std::queue<int> foundItems;
mutex lock_found_items;
int patternLength = 0;
int	textLength = 0;
int searchText = 0;
int counter = 0;
mutex lock_saved_text;
string saved = "";
int thread_amount_choice = 0;


// variables for output_finished_threads thread function
mutex lock_finished_thread;
condition_variable finished_thread_cv;
bool not_ready = false;
int finished_thread_number = 0;
int total_completed_thread;


void output_finished_threads() {

	cout << endl;
	while ((total_completed_thread + 1) != thread_amount_choice) {

		//if statment to fix error when while loop does not stop 
		unique_lock<mutex> finished_thread_lock(lock_finished_thread);


		//will not continue untill a notify has happened (stops thread from keep looping and taking up cpu usage)
		finished_thread_cv.wait(finished_thread_lock, [] { return not_ready; });


		// will notify what thread is done and then reset the variables
		cout << "Thread " << finished_thread_number << " Is DONE!" << endl;
		total_completed_thread++;
		not_ready = false;
	}
}



// NOT CHANGED SINCE LAST ASSIGNMENT
// this works out the max number of two ints (to make sure when we run the program it will move in positive numbers)
int max(int a, int b) {

	if (a < b) {

		return a;
	}
	else {

		return b;
	}
}



// NOT CHANGED SINCE LAST ASSIGNMENT
void badCharacterHeuristic(char *pat, int sizeofPattern, int badcharacter[NO_OF_CHARS]) {

	int i = 0;
	for (i; i < NO_OF_CHARS; i++) {

		//will set all of the chacters value in bad character equal to -1
		badcharacter[i] = -1;
	}


	for (i; i < sizeofPattern; i++) {

		// fill set the last apperance of the character
		badcharacter[(int)pat[i]] = i;
	}
}



//algorithm function to search text using boyer moore
int searchString(char *text, char *pattern, int start_search_point, int end_search_point, int thread_number) {

	//gets the length of the patten and text
	int patternLength = strlen(pattern);
	// set badcharacter to badcharacter[256] meaning the array's size is 256 
	int badcharacter[NO_OF_CHARS];
	badCharacterHeuristic(pattern, patternLength, badcharacter);
	int x = start_search_point;


	//wilst there is still more characters to look at, keep searching
	while (x <= end_search_point - patternLength) {

		int y = patternLength - 1;


		// this will run whilst the pattern character and the text character are the same so we can check a different character
		while (y >= 0 && pattern[y] == text[x + y]) {

			y--;
		}


		// if the pattern has been found
		if (y < 0) {

			//protects the found items vector
			lock_guard<mutex> lock(lock_found_items);


			//stores all found results in a queue in order 
			foundItems.push(x);
			x += (x + patternLength < end_search_point) ? patternLength - 1 - badcharacter[text[x + patternLength]] : 1; // this will move the pattern so that the next charcter in the text will line up with the characters last apperance in the pattern
		}
		else {

			//if the previous "if" is wrong then it will line up the "badCharacter" of the "text" with the character that last time it occured in the "pattern"
			x += max(1, y - badcharacter[text[x + y]]);
		}
	}


	//lock the mutex so it can notify and the finished_thread_number will not change or wait untill it is unlocked
	unique_lock<mutex> finished_thread_lock(lock_finished_thread);


	finished_thread_number = thread_number;
	not_ready = true;
	finished_thread_cv.notify_one();


	return 0;
}



//algorithm function to search text using brute force
int bruteForce(const string pattern, const string text, int start_search_point, int end_search_point, int thread_number) {

	//local variables for brute force
	patternLength = pattern.length();
	int x = start_search_point;


	// will it has not reach the end of its text it needs to look though keep looking
	while (x < end_search_point) {

		int y = 0;


		//whist Y is less than the text length (whist the whole pattern has not been found)
		// AND whilst the patterns (X+Y)th letter matches up with the texts Yth letter continue
		// if not return to the for loop and search the next letters;
		// if the amount of letters that match equal the size of the pattern then all of the pattern has been found:
		while (y < end_search_point && text[x + y] == pattern[y]) {

			y = y + 1;


			if (y == patternLength) {

				//protect the found items vector
				lock_guard<mutex> lock(lock_found_items);
				x++;
				y = 0;
				counter++;


				foundItems.push(x);
			}
		}


		x++;
	}


	//lock the mutex so it can notify and the finished_thread_number will not change or wait untill it is unlocked
	unique_lock<mutex> finished_thread_lock(lock_finished_thread);


	finished_thread_number = thread_number;
	not_ready = true;
	finished_thread_cv.notify_one();


	return 0;
}



// function to check how many characters to search per thread
int how_many_characters_per_thread(int number_of_threads, string text, string pattern) {

	int text_size = text.length();
	int pattern_size = pattern.length();
	int characters_per_thread = 0;


	// dont do work if you dont have to
	if (number_of_threads == 1) {

		return text_size;
	}
	else {

		characters_per_thread = (text_size / number_of_threads);
		return characters_per_thread;
	}
}




//load file chosen
int inFile(string file_choice, int thread_number ) {

	string file_name = "";
	thread_number++;
	string local_saved;


	// check what file was chosen
	if (file_choice == "1") {

		file_name = "250k_characters//250k_characters_part_" + to_string(thread_number) + ".txt";
	}
	else if (file_choice == "2") {

		file_name = "500k_characters//500k_characters_part_" + to_string(thread_number) + ".txt";
	}
	else if (file_choice == "3") {

		file_name = "750k_characters//750k_characters_part_" + to_string(thread_number) + ".txt";
	}
	else if (file_choice == "4") {

		file_name = "1_mill_characters//1_mill_characters_part_" + to_string(thread_number) + ".txt";
	}
	else if (file_choice == "5") {

		file_name = "1,25_mill_characters//1,25_mill_characters_part_" + to_string(thread_number) + ".txt";
	}


	//load and save as a string
	ifstream inFile(file_name);


	if (inFile.fail()) {

		cout << "File not loaded" << endl;
	}
	else {

		string text;
		

		while (getline(inFile, text)) {

			local_saved += " " + text;
		}
	}


	lock_guard<mutex> lock(lock_saved_text);
	saved += "" + local_saved;


	//close file loaded
	inFile.close();


	// clear the screen
	system("cls");


	return 0;
}



int main() {

	//local varaibles
	string brutePattern = "";
	char inputPattern[256] = "";
	// this is a string in order to prevent crashing if user enters a string instead of int
	string algorithmChoice = "0";
	string file_choice = "0";


	// as user what file they want to use (keep asking until valid choice)
	while (file_choice != "1"  && file_choice != "2" && file_choice != "3" && file_choice != "4" && file_choice != "5") {

		cout << endl << "1   -   File With 250k Words" << endl;
		cout << "2   -   File With 500k Words" << endl;
		cout << "3   -   File With 750k Words" << endl;
		cout << "4   -   File With 1 Mill Words" << endl;
		cout << "5   -   File With 1.25 Mill Words" << endl << endl;


		cout << "Enter the number of the file you want to search: " << flush;
		cin >> file_choice;
	}


	// clear the screen
	system("cls");


	//alert user its working
	cout << endl << "Loading Your File...";


	thread load_file_thread_vector[5];


	//load file using threads
	for (int i = 0; i < 5; i++) {

		load_file_thread_vector[i] = thread(inFile, file_choice, i);
	}


	//join load file threads to main thread
	for (int i = 0; i < 5; i++) {

		load_file_thread_vector[i].join();
	}


	// as how many threads the user wants to use and keep asking until valid input
	while (thread_amount_choice <= 0 || thread_amount_choice > 10) {

		cout << endl << "How many threads do you want to use (1-10): " << flush;
		cin >> thread_amount_choice;
	}


	// create vector for threads and thread vector size
	vector<thread> thread_vector;
	thread_vector.resize(thread_amount_choice);


	// ask user for what algorithm they want to use and keep asking untill valid input has been entered
	while (algorithmChoice != "1" && algorithmChoice != "2") {

		// clear the screen
		system("cls");


		cout << endl << "1   -   Brute Force" << endl;
		cout << "2   -   Boyer Moor" << endl << endl;
		cout << "Enter the number of the algorithm you want to use: " << flush;
		cin >> algorithmChoice;
	}


	the_clock::time_point start;
	the_clock::time_point end;


	// check what algorithm chosen and run related code
	if (algorithmChoice == "1") {

		// clear the screen
		system("cls");


		cout << endl << "Enter pattern you want to search for: " << flush;


		cin.ignore();
		getline(cin, brutePattern);


		// clear the screen
		system("cls");


		start = the_clock::now();
		int characters_per_thread = how_many_characters_per_thread(thread_vector.size(), saved, brutePattern);


		// create side thread
		thread output_completed_thread(output_finished_threads);


		//create threads 
		for (int i = 0; i < thread_vector.size(); i++) {

			int start_search_point = (characters_per_thread * i);
			int end_search_point = characters_per_thread * (i + 1) + (brutePattern.length() - 1); // overbite incase last letter of search point is start of the pattern


			//this removes the overbite and makes it so that it does not need to search anything unessary at the end
			if (end_search_point > saved.length()) {

				end_search_point = (saved.length() - brutePattern.length()) + 1;
			}


			thread_vector[i] = thread(bruteForce, brutePattern, saved, start_search_point, end_search_point, i);
		}


		// join created threads to main thread
		for (int i = 0; i < thread_vector.size(); i++) {

			thread_vector[i].join();
		}


		// join side thread to main thread
		output_completed_thread.join();
		end = the_clock::now();
	}
	else {

		// clear the screen
		system("cls");


		cout << endl << "Enter pattern you want to search for! ";


		cin.ignore();
		cin.getline(inputPattern, sizeof(inputPattern));


		// clear the screen
		system("cls");


		start = the_clock::now();
		int characters_per_thread = how_many_characters_per_thread(thread_vector.size(), saved, inputPattern);


		//create side thread
		thread output_completed_thread(output_finished_threads);


		// create threads
		for (int i = 0; i < thread_vector.size(); i++) {

			int start_search_point = (characters_per_thread * i);
			int end_search_point = characters_per_thread * (i + 1) + (strlen(inputPattern) - 1); // overbite incase last letter of search point is start of the pattern


			//this removes the overbite and makes it so that it does not need to search anything unessary at the end
			if (end_search_point > saved.length()) {

				end_search_point = (saved.length() - strlen(inputPattern)) + 1;
			}


			thread_vector[i] = thread(searchString, (char*)saved.c_str(), inputPattern, start_search_point, end_search_point, i);
		}


		//join threads to main thread
		for (int i = 0; i < thread_vector.size(); i++) {

			thread_vector[i].join();
		}


		//join side thread to main thread
		output_completed_thread.join();


		end = the_clock::now();
	}


	// calculate time taken
	auto time_taken = duration_cast<milliseconds>(end - start).count();


	// aleart user of infomation about search just ran
	cout << endl << endl << foundItems.size() << " occurrences of  '" << brutePattern << inputPattern << "'  found in your text file." << endl;
	cout << "It took " << time_taken << " ms. To find all your results." << endl;


	//pause the system so user can have time to look
	system("pause");
}