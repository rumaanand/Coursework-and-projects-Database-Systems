#include "stdafx.h"
#include <Windows.h>
#include "mysql.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include<stdlib.h>
#include <list>
#include<conio.h>
using namespace std;
MYSQL *conn;
time_t rawtime;
struct tm timeinfo;  // no longer a pointer

					 //converts string to number
int StringToNumber(const string &Text)
{
	stringstream ss(Text);
	int result;
	return ss >> result ? result : 0;
}

/*Login function. Returns Student Id as string*/
string Login()
{
	string psswrd, usrnm, rspns;
	MYSQL_RES *res_set;
	MYSQL_ROW row;

	mysql_query(conn, "SELECT * FROM student;");
	res_set = mysql_store_result(conn);

	if (res_set == NULL)
	{
		cout << "Could not access student table." << endl;
		return "";
	}

	int numrows = (int)mysql_num_rows(res_set);

	cout << "Enter username: "; //get user information
	getline(cin, usrnm);
	cout << "Enter password: ";
	getline(cin, psswrd);

	for (int i = 0; i < numrows; i++)
	{
		row = mysql_fetch_row(res_set);
		if (row != NULL)
		{
			if ((usrnm == row[1]) && (psswrd == row[2]))
			{
				cout << "Login Successful!\n\n" << endl;
				string ID = row[0]; //saves ID and clears results
				mysql_free_result(res_set);
				return ID; //returns student ID 
			}
		}
	}
	cout << "Invalid Credentials" << endl;
	mysql_free_result(res_set);
	return Login();
}

/*Page for enrolling students*/
int enroll(string ID, string currQuarter, string nextQuarter, int raw_year)
{
	string currYear, nextYear;

	currYear = to_string(raw_year);
	if (nextQuarter == "Q2") //only year difference is if Q1 is becoming Q2
		nextYear = to_string(raw_year + 1);
	else
		nextYear = currYear;

	cout << "\n\nENROLLMENT" << endl;
	cout << "---------------" << endl;
	cout << "\n1. Enroll for " + currQuarter + " " + currYear << endl;
	cout << "2. Enroll for " + nextQuarter + " " + nextYear << endl;
	cout << "\nPlease select the quarter you would like to enroll in courses for or enter 'M' to return to the Main Menu:" << endl;
	string input;

	string year, quarter;
	//Setting year and quarter equivalent to whatever preferred enrollment quarter is
	while (true)
	{
		getline(cin, input);

		if (input == "1")
		{
			quarter = currQuarter;
			year = currYear;
			break;
		}
		else if (input == "2")
		{
			quarter = nextQuarter;
			year = nextYear;
			break;
		}
		else if (input == "m" || input == "M")
			return 0;
		else
			cout << "\nPlease enter a valid input." << endl;
	}

	//with quarter and year chosen, onto the course selection

	MYSQL_RES *res_set;
	MYSQL_ROW row;
	string courseCode;

	string q = "CALL getAvailableCourses(" + ID + ", '" + quarter + "', '"
		+ year + "');";

	mysql_query(conn, q.c_str());

	int status = 0;
	bool coursesAvailable = false;
	bool courseOnList = false;

	/*To allow users to switch quarters or exit to menu instead of choosing
	course, the following two bools allow the do loop to complete before the
	function exits to prevent pointer issues with mysql_res*/
	bool toMenu = false;
	bool chooseQuarter = false;

	do //ensures call function returns properly with no redundant results
	{
		res_set = mysql_store_result(conn);

		if (res_set)
		{
			coursesAvailable = true;
			//list courses available for this given quarter
			cout << "\nCourses available during " << quarter << " " << year << endl;
			cout << "\nCourse ID\t\t" << "Course Name" << endl;

			int numrows = (int)mysql_num_rows(res_set);
			for (int i = 0; i < numrows; i++)
			{
				row = mysql_fetch_row(res_set);
				if (row != NULL)
					cout << row[0] << "\t\t" << row[1] << endl;
			}

			mysql_data_seek(res_set, 0);

			cout << "\nEnter the UoS Code for the course you would like to enroll in. To enroll in a different quarter, enter 'Q'. To exit to the Main Menu, enter 'M'." << endl;

			while (true) //get user input
			{
				getline(cin, input); //switch quarters
				if (input == "q" || input == "Q")
				{
					chooseQuarter = true;
					break;
				}

				else if (input == "m" || input == "M") // back to menu
				{
					toMenu = true;
					break;
				}

				else
				{
					//first, check whether user's code is even available
					int numrows = (int)mysql_num_rows(res_set);
					bool courseFull = false; //check for full course
					for (int j = 0; j < numrows; j++)
					{
						row = mysql_fetch_row(res_set);
						if (row != NULL)
						{
							if (row[0] == input) //if it is
							{
								courseOnList = true;
								int enroll = StringToNumber(row[2]); //converts string output to int
								int max = StringToNumber(row[3]);

								if (enroll >= max) //block enrollment if full
								{
									courseFull = true;
									cout << "Error: " << input << " is at max capacity. Please select a different course." << endl;
								}

								else
									courseCode = input; //else, save it to check prereqs later

								break; //stop checking rows now since something's found
							}
						}
					}

					mysql_data_seek(res_set, 0);

					if (courseFull) //get input again
						continue;

					else if (courseOnList) //if course is on list but not full, end loop
						break;

					else
						cout << "Invalid entry. Please enter a valid course code or option:" << endl;
				}
			}
		}
		else
		{
			if (!coursesAvailable) //if no courses found
			{
				mysql_free_result(res_set);

				cout << "No Courses available in this quarter. Enroll for a different quarter? (Y/N)" << endl;
				getline(cin, input);

				if (input == "y" || input == "Y") //enroll in other quarter
					chooseQuarter = true;
				else
					toMenu = true;
			}
		}

		mysql_free_result(res_set);
		status = mysql_next_result(conn);

		if (status > 0)
		{
			cout << "Results error." << endl;
			break;
		}
	} while (status == 0);

	if (toMenu) //now go to menu
		return 0;
	if (chooseQuarter) //or choose quarter
		return enroll(ID, currQuarter, nextQuarter, raw_year);

	if (courseOnList) //check the prereqs
	{
		string prereq_query = "CALL prerequisitesCheck('" + courseCode
			+ "', " + ID + ");";
		mysql_query(conn, prereq_query.c_str());

		status = 0;
		bool prereqsDone = true;

		do
		{
			res_set = mysql_store_result(conn);

			if (res_set) //if result, then prereqs were not fulfilled
			{
				int numrows = (int)mysql_num_rows(res_set);
				for (int i = 0; i < numrows; i++)
				{
					row = mysql_fetch_row(res_set);
					if (row != NULL)
					{
						if (i == 0) //once verified that first row isn't null, prereqs must not be done
						{
							prereqsDone = false;
							cout << "Error: cannot enroll in " << courseCode <<
								" because the following prerequisites have not been fulfilled:" << endl;
						}
						cout << row[0] << "\t" << row[1] << endl;
					}
				}
			}

			mysql_free_result(res_set);
			status = mysql_next_result(conn);

			if (status > 0)
			{
				cout << "Results error." << endl;
				break;
			}
		} while (status == 0);

		if (prereqsDone) //if prereqs are all good, finally call enroll
		{
			string add_query = "CALL addCourse('" + courseCode + "', " +
				ID + ", '" + quarter + "', '" + year + "');";
			mysql_query(conn, add_query.c_str());
			cout << "Successfully enrolled in course " << courseCode << endl;
		}
	}

	cout << "\nWould you like to enroll in another course? (Y/N)" << endl;
	getline(cin, input);

	if (input == "n" || input == "N")
		return 0;
	else
		return enroll(ID, currQuarter, nextQuarter, raw_year);

}

void lookup_course(string code, string quarter, string year, string grade)
{
	MYSQL_RES *lookup_res;
	MYSQL_ROW row;
	string courseName, courseEnroll, courseEnrollMax, prof;
	string q = "CALL courseinfo('" + code + "','" + quarter + "','" + year + "');";

	mysql_query(conn, q.c_str());
	int status = 0;

	do //ensures call function returns properly with no redundant results
	{
		lookup_res = mysql_store_result(conn);

		if (lookup_res)
		{
			row = mysql_fetch_row(lookup_res);
			courseName = row[0];
			courseEnroll = row[1];
			courseEnrollMax = row[2];
			prof = row[3];
		}

		mysql_free_result(lookup_res);
		status = mysql_next_result(conn);

		if (status > 0)
		{
			cout << "Results error." << endl;
			break;
		}
	} while (status == 0);

	cout << "\nCOURSE DETAILS" << endl; //print it out
	cout << "\nCourse ID:" << "\t\t" << code << endl;
	cout << "Course Name:" << "\t\t" << courseName << endl;
	cout << "Instructor:" << "\t\t" << prof << endl;
	cout << "Quarter Enrolled:" << "\t" << quarter << " " << year << endl;
	cout << "Total Enrollment:" << "\t" << courseEnroll << endl;
	cout << "Enrollment Capacity:" << "\t" << courseEnrollMax << endl;
	cout << "Grade Received:" << "\t\t" << grade << endl;

	return;
}

int transcript(string ID)
{
	MYSQL_RES *transcript_res;
	MYSQL_ROW r;

	string q = "SELECT * FROM transcript where StudId = " + ID +
		" ORDER BY Year desc, Semester asc, Grade asc;";
	mysql_query(conn, q.c_str());
	transcript_res = mysql_store_result(conn);

	if (transcript_res == NULL)
	{
		cout << "Could not retrieve transcript information." << endl;
		return 0;
	}

	int numrows = (int)mysql_num_rows(transcript_res);

	cout << "\n\nSTUDENT TRANSCRIPT" << endl; //prints out transcript
	cout << "--------------------------" << endl;
	cout << "\n" << "UoS Code" << "\t" << "Quarter Taken" << "\t" <<
		"Grade" << "\n" << endl;

	for (int i = 0; i < numrows; i++)
	{
		r = mysql_fetch_row(transcript_res);
		if (r != NULL)
		{
			string grade = "N/A"; //for current classes, no grade
			if (r[4] != NULL)
				grade = r[4];
			cout << r[1] << "\t" << r[2] << " " << r[3] << "\t\t" << grade << endl;
		}
	}

	cout << "\nEnter a UoS Code for information on the course or enter 'M' to return to the Main Menu." << endl;
	while (true) //repeat infinitely until exiting function
	{
		string input;
		getline(cin, input);

		if (input == "M" || input == "m") //quits to main menu
		{
			mysql_free_result(transcript_res);
			return 0;
		}
		else
		{
			string courseCode = "none";
			string courseYear, courseQ, courseGrade;

			mysql_data_seek(transcript_res, 0); //resets fetch_row pointer
			for (int i = 0; i < numrows; i++)
			{
				r = mysql_fetch_row(transcript_res);
				if (r != NULL)
				{
					if (input == r[1]) //find the course that matches the input
					{
						courseCode = input; //pull relevant values
						courseQ = r[2];
						courseYear = r[3];
						courseGrade = "N/A";
						if (r[4] != NULL)
							courseGrade = r[4];
						break;
					}
				}
			}


			if (courseCode != "none") //if matching course found
			{
				lookup_course(courseCode, courseQ, courseYear, courseGrade);

				cout << "\nWould you like to search for another course? (Y/N)" << endl;
				getline(cin, input);

				if (input == "N" || input == "n")
					return 0;

				else
					return transcript(ID);

			}
			else
				cout << "Please enter a valid course number or command." << endl; //loops back if no course found
		}
	}
}

//Student Info page
int personalDetails(string ID)
{
	MYSQL_RES *details_res;
	MYSQL_ROW row;

	string q = "SELECT * FROM student WHERE Id = '" + ID + "';";
	mysql_query(conn, q.c_str());
	details_res = mysql_store_result(conn);

	if (details_res == NULL)
	{
		cout << "Could not access personal details." << endl;
		return 0;
	}

	row = mysql_fetch_row(details_res);

	cout << "\n\nPERSONAL DETAILS" << endl;
	cout << "--------------------" << endl;
	cout << "\nName:" << "\t\t" << row[1] << endl;
	cout << "Student ID:" << "\t" << ID << endl;
	cout << "Address:" << "\t" << row[3] << endl;
	cout << "\nPlease select an option:" << endl;
	cout << "1. Change password" << endl;
	cout << "2. Change address" << endl;
	cout << "3. Return to Main Menu" << endl;

	mysql_free_result(details_res);

	while (true) //keep asking until correct response
	{
		string input;
		getline(cin, input);

		if (input == "1")
		{
			string newPass = "";
			cout << "\nPlease enter a new password below:" << endl;
			getline(cin, newPass);
			while (newPass == "") //must not be empty
			{
				cout << "\nPassword cannot be blank. Please enter a new password below:" << endl;
				getline(cin, newPass);
			}
			string p_query = "UPDATE student SET Password = '" + newPass +
				"' WHERE Id = " + ID + ";";
			mysql_query(conn, p_query.c_str());
			cout << "\nPassword updated successfully. Return to Main Menu? (Y/N)" << endl;
			getline(cin, input);
			if (input == "Y" || input == "y")
				return 0;

			else
				return personalDetails(ID);

		}
		else if (input == "2")
		{
			string newAddress;
			cout << "\nPlease enter your updated address:" << endl;
			getline(cin, newAddress);

			while (newAddress == "") //must not be empty
			{
				cout << "\nAddress cannot be blank. Please enter your updated address below:" << endl;
				getline(cin, newAddress);
			}
			string p_query = "UPDATE student SET Address = '" + newAddress +
				"' WHERE Id = " + ID + ";";
			mysql_query(conn, p_query.c_str());
			cout << "\nAddress updated successfully. Return to Main Menu? (Y/N)" << endl;
			getline(cin, input);
			if (input == "Y" || input == "y")
				return 0;

			else
				return personalDetails(ID);
		}
		else if (input == "3")
			return 0;
		else
			cout << "Please select a valid option." << endl;
	}
}

int withdraw(string ID)
{
	MYSQL_RES *course_res,*res2;
	MYSQL_ROW r,r2;

	//Accesses all courses that currently have NULL grades
	string query = "SELECT UoSCode, UoSName, Semester, Year FROM transcript inner join unitofstudy using(UoSCode) WHERE StudId = "
		+ ID + " and Grade is NULL;";
	mysql_query(conn, query.c_str());
	course_res = mysql_store_result(conn);

	if (course_res == NULL)
	{
		cout << "No courses found." << endl;
		return 0;
	}
	cout << "\n\nCOURSE WITHDRAWAL" << endl;
	cout << "---------------------" << endl;
	cout << "\nCode \t Course Name\n" << endl;

	int numrows = (int)mysql_num_rows(course_res);
	for (int i = 0; i < numrows; i++)
	{
		r = mysql_fetch_row(course_res);
		if (r != NULL)
			cout << r[0] << "\t" << r[1] << endl;
	}
	mysql_data_seek(course_res, 0); //reset sql row pointer

	cout << "\nEnter a UoS Code to withdraw from the course or enter 'M' to return to the Main Menu:" << endl;
	string input, quarter, year;
	bool validCourse = false;

	while (!validCourse)
	{
		getline(cin, input);

		if (input == "m" || input == "M") //exits to main menu
		{
			mysql_free_result(course_res);
			return 0;
		}

		for (int i = 0; i < numrows; i++)
		{
			r = mysql_fetch_row(course_res);
			if (r != NULL)
			{
				if (input == r[0]) //if course is on list, save info and break
				{
					quarter = r[2];
					year = r[3];
					mysql_free_result(course_res);
					validCourse = true;
					break;
				}
			}
		}

		if (!validCourse) //else, get different input from user
			cout << "Invalid entry. Please enter a course code from the list above or 'M' to exit:" << endl;

		mysql_data_seek(course_res, 0);
	}

	//Calls withdraw procedure which decrements enrollment and removes course
	string delete_query = "CALL withdrawStudent(" + ID + ", '" + input + "', '" +
		quarter + "', '" + year + "');";
	mysql_query(conn, delete_query.c_str());

	cout << "\n Successfully withdrawn from " << input << ".\n";
	string res = "select @message";
	mysql_query(conn, res.c_str());
	res2 = mysql_store_result(conn);
	r2 = mysql_fetch_row(res2);
	cout << r2[0];
	
	cout << "\nWithdraw from another course ? (Y / N)" << endl;
	getline(cin, input);

	if (input == "n" || input == "N")
		return 0;

	else
		return withdraw(ID);
}

//Serves as a hub. Recurses continuously until the user quits
int MainMenu(string ID)
{
	//Establish the current quarter
	time(&rawtime);
	localtime_s(&timeinfo, &rawtime);

	int month = timeinfo.tm_mon;
	int year = timeinfo.tm_year;

	string currQuarter, nextQuarter;

	if (month >= 8 && month <= 10)
	{
		currQuarter = "Q1";
		nextQuarter = "Q2";
	}

	else if (month > 10 || month <= 1)
	{
		currQuarter = "Q2";
		nextQuarter = "Q3";
	}

	else if (month > 1 && month <= 4)
	{
		currQuarter = "Q3";
		nextQuarter = "Q4";
	}

	else
	{
		currQuarter = "Q4";
		nextQuarter = "Q1";
	}

	year = year + 1900; //convert to present time
	string currYear = to_string(year);

	//Pull up the student's current course schedule
	MYSQL_RES *res_set1;
	MYSQL_ROW row1;

	string str = "select u.uoSCode, u.UoSName from unitofstudy u, transcript t where u.UoSCode = t.UoSCode and t.StudID = '"
		+ ID + "' and t.semester='" + currQuarter + "' and t.Year = '" + currYear +
		"' and t.Grade is NULL;";
	mysql_query(conn, str.c_str());
	res_set1 = mysql_store_result(conn);

	if (res_set1 == NULL)
	{
		cout << "No courses found." << endl;
	}

	else
	{
		int numrows1 = (int)mysql_num_rows(res_set1);

		cout << "\n" << currQuarter << " " << currYear << " Schedule\n";
		cout << "\n" << "ID" << "\t" << "Course Name" << "\n";

		for (int i = 0; i < numrows1; i++) //Prints out the course list
		{
			row1 = mysql_fetch_row(res_set1);
			if (row1 != NULL)
			{
				cout << "\n";
				cout << row1[0] << "\t" << row1[1];
			}
		}
	}

	mysql_free_result(res_set1);

	cout << "\n\n\tMAIN MENU" << endl; //Print options
	cout << "--------------------------" << endl;
	cout << "1. Transcript" << endl;
	cout << "2. Enroll" << endl;
	cout << "3. Withdraw" << endl;
	cout << "4. Personal Details" << endl;
	cout << "5. Logout" << endl;
	cout << "\n Please select an option or enter 'Q' to quit:" << endl;


	while (true) //repeat infinitely until valid input provided
	{
		string input;
		getline(cin, input);
		if (input == "1")
		{
			transcript(ID);
			return MainMenu(ID);
		}
		else if (input == "2")
		{
			enroll(ID, currQuarter, nextQuarter, year);
			return MainMenu(ID);
		}
		else if (input == "3")
		{
			withdraw(ID);
			return MainMenu(ID);
		}
		else if (input == "4")
		{
			personalDetails(ID);
			return MainMenu(ID);
		}
		else if (input == "5")
		{
			string newID = Login();
			return MainMenu(newID);
		}
		else if (input == "q" || input == "Q")
		{
			return 0;
		}
		else
		{
			cout << "Please select a valid option." << endl;
		}
	}
}


int main()

{
	conn = mysql_init(NULL);
	mysql_real_connect(
		conn,
		"localhost",
		"root", //change to whatever your default username is
		"12345", //change to whatever your default password is
		"project3-nudb",
		0, //whatever port you choose
		NULL,
		CLIENT_MULTI_RESULTS);
	cout << "\n********NORTHWESTERN UNIVERSITY CAESAR********\n";

	string ID;
	ID = Login();
	MainMenu(ID);

	mysql_close(conn);
	return 0;
}