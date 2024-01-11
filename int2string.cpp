#include <sstream>
#include <iostream>

using namespace std;

int main()
{
    //int to string
    int a = 45678;
    ostringstream oss;
    oss << a;
    cout << "str=" << oss.str() << endl;

    //string to int
    int b = 0;
    string b_str = "787878";
    istringstream iss(b_str);
    iss >> b;
    cout << "b=" << b << endl;

    stringstream ss;
    ss << "12345678";
    int val;
    ss >> val;
    cout << "val=" << val <<endl;
    ss.clear();
    ss << 7777;
    cout << "s=" << ss.str() << endl;

    return 0;
}