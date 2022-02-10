#include <iostream>
#include <conio.h>
#include <functional>
#include <fstream>
#include <filesystem>
#include <string>
#include <chrono>
#include <algorithm>
#include <gtest/gtest.h>
#include "thread_pool.h"

#define MY_TEST_PATH "C:\\Users\\Hubert Wachowiak\\Desktop\\DirectoriesTest"

using namespace std;
namespace fs = std::filesystem;

typedef unsigned int uint;
typedef unsigned int ulint;
typedef unsigned int usint;

struct counter
{
    uint dirs;
    ulint files; 
    ulint lines; 
    ulint full;
    ulint empty; 
    ulint words; 
    ulint letters; 



};

const usint max_nr_of_threads = thread::hardware_concurrency();
counter nr_of;
mutex C;

usint set_nr_of_threads(usint requested_nr_of_threads)
{
    if (requested_nr_of_threads < max_nr_of_threads && requested_nr_of_threads > 0) return requested_nr_of_threads; 
    else if (requested_nr_of_threads <= 0) return 1;                                                                
    else return max_nr_of_threads;                                                                                 
}

void counter_fun(const string& file_pathname)
{
    fstream  reader;
    string line_buffer;
    string word_buffer;

    reader.open(file_pathname);
    while (!reader.eof())
    {
        line_buffer.clear();
        getline(reader, line_buffer, '\n');
       
        if (!line_buffer.empty())
        {
            size_t start = 0;
            size_t stop = 0;
            
            do
            {
                
                start = line_buffer.find_first_of("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM", stop);
                if (start == string::npos) break;
                stop = line_buffer.find_first_not_of("qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM", start);
                word_buffer = line_buffer.substr(start, stop - start);
                C.lock();
                nr_of.words++; 
                C.unlock();
                
                uint a = count_if(word_buffer.begin(), word_buffer.end(), [](char c) {
                    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) return true;   
                    else return false;
                    });

                C.lock();
                nr_of.letters += a; 
                C.unlock();

            } while (stop != string::npos);

            C.lock();
            nr_of.full++; 
            C.unlock();
        }
      
        else
        {
            C.lock();
            nr_of.empty++; 
            if (!nr_of.full) nr_of.empty = 0; 
            C.unlock();
        }
    }
}

void analyze_path(const fs::path& parent, usint requested_nr_of_threads)
{
    
    usint used_nr_of_threads = set_nr_of_threads(requested_nr_of_threads);
   
    thread_pool tp(used_nr_of_threads);

    for (auto const& dir_entry: std::filesystem::recursive_directory_iterator{ parent })
    {
        
        fs::path subpath(dir_entry.path());
        fs::file_status subpath_stat = fs::status(subpath);
        if (fs::is_directory(subpath_stat)) nr_of.dirs++;     
        else
        {
            nr_of.files++;                                            
             
            tp.enqueue([subpath] {
                cout << "start " << endl;
                string pathname = subpath.string();
                counter_fun(pathname);                               
                cout << "stop" << endl;
                });
        }
    }
}


int main()
{
    fs::path files{ MY_TEST_PATH };

    cout << "Maximum number of threads to use: " << max_nr_of_threads << endl;
    cout << "Tested directory: \"DirectoriesTest\"" << endl;

    for (usint i = 1; i <= max_nr_of_threads; i++)
    {
        nr_of = { 0,0,0,0,0,0,0 };
        cout << "Performance results for " << i << " threads:" << endl;
        auto start = chrono::steady_clock::now();
        analyze_path(files, i);
        auto stop = chrono::steady_clock::now();
        chrono::duration<double> elapsed_time = stop - start;
        cout << "Required time: " << elapsed_time.count() << endl;
    }

    nr_of.lines = nr_of.full + nr_of.empty;
    cout << "There are: " << endl;
    cout << nr_of.files << " files" << '\n';                   
    cout << nr_of.dirs << " directories" << '\n';
    cout << nr_of.lines << " lines" << '\n';
    cout << nr_of.full << " full lines" << '\n';
    cout << nr_of.empty << " empty lines" << '\n';
    cout << nr_of.words << " words" << '\n';
    cout << nr_of.letters << " letters" << '\n';

    testing::InitGoogleTest();
    RUN_ALL_TESTS();
    return 0;
}

TEST(ThreadLimits, ThreadLimitExceeded)
{
    ASSERT_TRUE(set_nr_of_threads(-23) <= max_nr_of_threads);
    ASSERT_TRUE(set_nr_of_threads(0) <= max_nr_of_threads);
    ASSERT_TRUE(set_nr_of_threads(2) <= max_nr_of_threads);
    ASSERT_TRUE(set_nr_of_threads(765753) <= max_nr_of_threads);
}

TEST(CountingFilesTest, EmptyDirectoryTest)
{
    nr_of = { 0,0,0,0,0,0,0 };
    analyze_path("C:\\Users\\Hubert Wachowiak\\Desktop\\DirectoriesTest\\Empty", 4);
    EXPECT_EQ(nr_of.dirs, 0);
    EXPECT_EQ(nr_of.files, 0);
}

TEST(CountingFilesTest, TestRecursiveDirectory)
{
    nr_of = { 0,0,0,0,0,0,0 };
    analyze_path("C:\\Users\\Hubert Wachowiak\\Desktop\\DirectoriesTest", 4);
    EXPECT_EQ(nr_of.dirs, 12);
    EXPECT_EQ(nr_of.files, 11);
}

TEST(CountingInFileTest, EmptyFileTest) 
{
    nr_of = { 0,0,0,0,0,0,0 };
    counter_fun("C:\\Users\\Hubert Wachowiak\\Desktop\\DirectoriesTest\\TestFiles\\Empty.txt");
    EXPECT_EQ(nr_of.empty, 0);
    EXPECT_EQ(nr_of.full, 0);
    EXPECT_EQ(nr_of.letters, 0);
    EXPECT_EQ(nr_of.words, 0);
}

TEST(CountingInFileTest, BigFileOnlyWords)
{
    nr_of = { 0,0,0,0,0,0,0 };
    counter_fun("C:\\Users\\Hubert Wachowiak\\Desktop\\DirectoriesTest\\TestFiles\\LoremIpsum10K_Words.txt");
    EXPECT_EQ(nr_of.empty, 113);
    EXPECT_EQ(nr_of.full, 114);
    EXPECT_EQ(nr_of.letters, 55683);
    EXPECT_EQ(nr_of.words, 10000);
}

TEST(CountingInFileTest, MixedNumbersWords)
{
    nr_of = { 0,0,0,0,0,0,0 };
    counter_fun("C:\\Users\\Hubert Wachowiak\\Desktop\\DirectoriesTest\\TestFiles\\MixedWordsAndNumbers.txt");
    EXPECT_EQ(nr_of.empty, 1);
    EXPECT_EQ(nr_of.full, 4);
    EXPECT_EQ(nr_of.letters, 74);
    EXPECT_EQ(nr_of.words, 18);





}

