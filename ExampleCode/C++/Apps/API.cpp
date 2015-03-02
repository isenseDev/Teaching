#include <iostream>
#include <string>                    // std::string, std::to_string;
#include "include/API.h"

// To avoid poluting the namespace, and also to avoid typing std:: everywhere.
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::map;
using std::string;
using std::to_string;
using std::vector;


// Default constructor
iSENSE::iSENSE()
{
  // Set these to default values for error checking.
  upload_URL = "URL";
  get_URL = "URL";
  get_UserURL = "URL";
  title = "title";
  project_ID = "empty";
  dataset_ID = "empty";
  contributor_key = "key";
  contributor_label = "label";
  email = "email";
  password = "password";
}


// Constructor with parameters
iSENSE::iSENSE(string proj_ID, string proj_title, string label, string contr_key)
{
  set_project_ID(proj_ID);
  set_project_title(proj_title);
  set_project_label(label);
  set_contributor_key(contr_key);
}


// Similar to the constructor with parameters, but can be called at anytime
void iSENSE::set_project_all(string proj_ID, string proj_title, string label, string contr_key)
{
  set_project_ID(proj_ID);
  set_project_title(proj_title);
  set_project_label(label);
  set_contributor_key(contr_key);
}


// This function should be called by the user, and should set up all the fields.
void iSENSE::set_project_ID(string proj_ID)
{
  // Set the Project ID, and the upload/get URLs as well.
  project_ID = proj_ID;
  upload_URL = devURL + "/projects/" + project_ID + "/jsonDataUpload";
  get_URL = devURL + "/projects/" + project_ID;
  get_project_fields();
}


// The user should also set the project title
void iSENSE::set_project_title(string proj_title)
{
  title = proj_title;
}


// This one is optional, by default the label will be "cURL".
void iSENSE::set_project_label(string proj_label)
{
  contributor_label = proj_label;
}


// As well as the contributor key they will be using
void iSENSE::set_contributor_key(string proj_key)
{
  contributor_key = proj_key;
}


// Users should never have to call this method, as it is possible to pull datasets and
// compare dataset names to get the dataset_ID.
// Users should instead use the append byName methods.
void iSENSE::set_dataset_ID(string proj_dataset_ID)
{
  dataset_ID = proj_dataset_ID;
}


// Sets both email & password at once. Checks for valid email / password.
bool iSENSE::set_email_password(string proj_email, string proj_password)
{
  email = proj_email;
  password = proj_password;

  if(get_check_user())
  {
    cout << "\nEmail and password are valid.\n";
    return true;
  }

  cerr << "\nEmail and password are **not** valid.\n";
  cerr << "Try entering your password again.\n";
  cerr << "You also need to have created an account on iSENSE / rSENSE.\n";
  cerr << "You can do so here: http://rsense-dev.cs.uml.edu/users/new \n\n";

  return false;
}


// Extra function that the user can call to just generate an ISO 8601 timestamp
// It does not push back to the map of vectors. It merely returns a string,
// that users may grab and then send off to the push_back function.
string iSENSE::generate_timestamp(void)
{
  time_t time_stamp;
  time(&time_stamp);
  char buffer[sizeof "2011-10-08T07:07:09Z"];

  // Generates the timestamp, stores it in buffer.
  // Timestamp is in the form of Year - Month - Day -- Hour - Minute - Seconds
  strftime(buffer, sizeof buffer, "%Y-%m-%dT%H:%M:%SZ", gmtime(&time_stamp));

  string cplusplus_timestamp(buffer);   // Converts char array (buffer) to C++ string

  return cplusplus_timestamp;
}


// Resets the object and clears the map.
void iSENSE::clear_data(void)
{
  // Set these to default values
  upload_URL = "URL";
  get_URL = "URL";
  get_UserURL = "URL";
  title = "title";
  project_ID = "empty";
  contributor_key = "key";
  contributor_label = "label";
  email = "email";
  password = "password";

  map_data.clear();   // Clear the map_data

  /* Clear the picojson objects:
   * object: upload_data, fields_data;
   *  value: get_data, fields;
   *  array: fields_array;
   */

  // Under the hood picojson::objects are STL maps and picojson::arrays are STL vectors.
  upload_data.clear();
  fields_data.clear();    // These are technically STL maps
  owner_info.clear();

  // Uses picojson's = operator to clear the get_data object and the fields object.
  value new_object;
  get_data = new_object;
  fields = new_object;

  // Clear the field array (STL vector)
  fields_array.clear();
  media_objects.clear();
  data_sets.clear();
}


// Adds a single string to the map, which keeps track of the data to be uploaded.
void iSENSE::push_back(string field_name, string data)
{
  // Add the piece of data to the back of the vector with the given field name.
  map_data[field_name].push_back(data);
}


// Add a field name / vector of strings (data) to the map.
void iSENSE::push_vector(string field_name, vector<string> data)
{
  // This will store a copy of the vector<string> in the map.
  // If you decide to add more data, you will need to use the push_back function.
  map_data[field_name] = data;
}


// Searches for projects with the search term.
// Returns a vector with projects that show up.
vector<string> iSENSE::get_projects_search(string search_term)
{
  string get_search = devURL + "/projects?utf8=true&search=" + search_term
  + "&sort=updated_at&order=DESC";

  // Vector of project titles.
  vector<string> project_titles;

  // This project will try using CURL to make a basic GET request to rSENSE
  CURL *curl = curl_easy_init();          // cURL object
  long http_code;                         // HTTP status code
  MEMFILE* json_file = memfopen();        // Writing JSON to this file.
  char error[256];                        // Errors get written here

  if(curl)
  {
    // Set the GET URL, in this case the one be created above using the user's
    // email & password.
    curl_easy_setopt(curl, CURLOPT_URL, get_search.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error);    // Write errors to the array "error"

    // From the picojson example, "github-issues.cc". Used  for writing the JSON to a file.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json_file);

    // Perform the request
    curl_easy_perform(curl);

    // We can actually get the HTTP response code from cURL, so let's do that to check for errors.
    http_code = 0;

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // cout the http code.
    cout << "\nhttp code was: " << http_code << endl << endl;

    /*
     *        The iSENSE API gives us one response code to check against:
     *        Success: 200 OK
     *        If we do not get a code 200 from iSENSE, something went wrong.
     */

    // If we do not get a code 200, or cURL quits for some reason, we didn't successfully
    // get the project's fields.
    if(http_code != 200)
    {
      cerr << "\nProject search failed.\n";

      // Clean up cURL and close the memfile
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      memfclose(json_file);

      return project_titles;
    }

    // We got a code 200, so try and parse the JSON into a PICOJSON object.
    // Error checking for the parsing occurs below.
    string errors;
    value projects_json;

    // This will parse the JSON file.
    parse(projects_json, json_file->data, json_file->data + json_file->size, &errors);

    // If we have errors, print them out and quit.
    if(errors.empty() != true)
    {
      cerr << "\nError parsing JSON file in method: get_projects_search(string search_term)\n";
      cerr << "Error was: " << errors << endl;

      // Clean up cURL and close the memfile
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      memfclose(json_file);

      return project_titles;
    }

    // Convert the JSON array (projects_json) into a vector of strings
    // (strings would be project titles);
    array projects_array = projects_json.get<array>();
    array::iterator it, the_begin, the_end;

    the_begin = projects_array.begin();
    the_end = projects_array.end();

    // Check and see if the projects_title JSON array is empty
    if(the_begin == the_end)
    {
      // Print an error and quit, we can't make a vector of project titles to return
      // if the JSON array ended up empty. Probably wasn't any projects with that search term.
      cerr << "\nProject title array is empty.\n";

      // Clean up cURL and close the memfile
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      memfclose(json_file);

      return project_titles;  // this is an empty vector
    }

    // If we make here, we can make a vector of project titles and return that to the user.
    for(it = projects_array.begin(); it != projects_array.end(); it++)
    {
      // Get the current object
      object obj = it->get<object>();

      // Grab the field name
      string name = obj["name"].get<string>();

      // Push the name back into the vector.
      project_titles.push_back(name);
    }

    // Clean up cURL and close the memfile
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    memfclose(json_file);

    return project_titles;
  }

  // If we get here, this returns an empty vector.
  // So if curl fails, an empty vector gets returned.
  cerr << "\nCurl failed, so an empty vector was returned.\n";
  return project_titles;
}


// Checks to see if the email / password is valid
bool iSENSE::get_check_user()
{
  // First check to see if the email and password have been set.
  if(email == "email" || password == "password")
  {
    cerr << "Please set an email & password for this project.\n";
    return false;
  }
  else if(email.empty() || password.empty())
  {
    cerr << "Please set an email & password for this project.\n";
    return false;
  }

  // If we get here, an email and password have been set, so do a GET using
  // the email & password.
  get_UserURL = devURL + "/users/myInfo?email=" + email + "&password=" + password;

  // This project will try using CURL to make a basic GET request to rSENSE
  CURL *curl = curl_easy_init();          // cURL object
  long http_code;                         // HTTP status code

  if(curl)
  {
    // Set the GET URL, in this case the one be created above using the user's
    // email & password.
    curl_easy_setopt(curl, CURLOPT_URL, get_UserURL.c_str());

    // Disable libcURL's desire to output to the screen by giving it a
    // write function which basically does nothing but returns the size of what
    // would have been outputted.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, suppress_output);

    // Perform the request
    curl_easy_perform(curl);

    // We can actually get the HTTP response code from cURL, so let's do so to check for errors.
    http_code = 0;

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    /*
     *      The iSENSE API gives us two response codes to check against:
     *      Success: 200 OK
     *      Failure: 401 Unauthorized
     */

    if(http_code == 200)
    {
      // Clean up cURL
      curl_easy_cleanup(curl);
      curl_global_cleanup();

      // Return success.
      return true;
    }

    // Clean up cURL
    curl_easy_cleanup(curl);
    curl_global_cleanup();
  }

  // If curl fails, return false.
  return false;
}


// This is pretty much straight from the GET_curl.cpp file.
bool iSENSE::get_project_fields()
{
  // Detect errors. We need a valid project ID before we try and perform a GET request.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "Error - project ID not set!\n";
    return false;
  }

  get_URL = devURL + "/projects/" + project_ID;

  // This project will try using CURL to make a basic GET request to rSENSE
  // It will then save the JSON it recieves into a picojson object.
  CURL *curl = curl_easy_init();      // cURL object
  long http_code;                     // HTTP status code
  MEMFILE* json_file = memfopen();    // Writing JSON to this file.
  char error[256];                    // Errors get written here

  if(curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, get_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error);    // Write errors to the array "error"

    // From the picojson example, "github-issues.cc". Used  for writing the JSON to a file.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json_file);

    // Perform the request, res will get the return code
    curl_easy_perform(curl);

    // We can actually get the HTTP response code from cURL, so let's do so to check for errors.
    http_code = 0;

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    /*
     *  The iSENSE API gives us one response code to check against:
     *  Success: 200 OK
     *  Failure: 404 Not Found
     */

    // If we do not get a code 200, or cURL quits for some reason, we didn't successfully
    // get the project's fields.
    if(http_code != 200)
    {
      cerr << "\nGET project fields failed.\n";
      cerr << "Is the project ID you entered valid?\n";

      // Clean up cURL and close the memfile
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      memfclose(json_file);

      return false;
    }

    // We got a code 200, so try and parse the JSON into a PICOJSON object.
    // Error checking for the parsing occurs below.
    string errors;

    // This will parse the JSON file.
    parse(get_data, json_file->data, json_file->data + json_file->size, &errors);

    // If we have errors, print them out and quit.
    if(errors.empty() != true)
    {
      cerr << "\nError parsing JSON file in method: get_project_fields()\n";
      cerr << "Error was: " << errors;
      return false;
    }

    // Save the fields to the field array
    fields = get_data.get("fields");
    fields_array = fields.get<array>();
  }

  // Clean up cURL and close the memfile
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  memfclose(json_file);

  // Return true as we were able to successfully get the project's fields.
  return true;
}


// Given that a project ID has been set, this function
// makes a GET request and saves all the datasets & media objects
// into two picojson arrays.
// It will also update the fields for that given project ID
bool iSENSE::get_datasets_and_mediaobjects()
{
  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return false;
  }

  // The "?recur=true" will make iSENSE return:
  // ALL datasets in that project.
  // ALL media objects in that project
  // And owner information.
  get_URL = devURL + "/projects/" + project_ID + "?recur=true";

  // This project will try using CURL to make a basic GET request to rSENSE
  // It will then save the JSON it recieves into a picojson object.
  CURL *curl = curl_easy_init();      // cURL object
  long http_code;                     // HTTP status code
  MEMFILE* json_file = memfopen();    // Writing JSON to this file.
  char error[256];                    // Errors get written here

  if(curl)
  {
    curl_easy_setopt(curl, CURLOPT_URL, get_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error);    // Write errors to the array "error"

    // From the picojson example, "github-issues.cc". Used  for writing the JSON to a file.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, memfwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json_file);

    // Perform the request, res will get the return code
    curl_easy_perform(curl);

    // We can actually get the HTTP response code from cURL, so let's do so to check for errors.
    http_code = 0;

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    /*
     *  The iSENSE API gives us one response code to check against:
     *  Success: 200 OK
     *  Failure: 404 Not Found
     */

    // If we do not get a code 200, or cURL quits for some reason, we didn't successfully
    // get the project's fields.
    if(http_code != 200)
    {
      cerr << "\nGET project fields failed.\n";
      cerr << "Is the project ID you entered valid?\n";

      // Clean up cURL and close the memfile
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      memfclose(json_file);

      return false;
    }

    // We got a code 200, so try and parse the JSON into a PICOJSON object.
    // Error checking for the parsing occurs below.
    string errors;

    // This will parse the JSON file.
    parse(get_data, json_file->data, json_file->data + json_file->size, &errors);

    // If we have errors, print them out and quit.
    if(errors.empty() != true)
    {
      cerr << "\nError parsing JSON file in method: get_datasets_and_mediaobjects()\n";
      cerr << "Error was: " << errors;
      return false;
    }

    // Save the fields to the field array
    fields = get_data.get("fields");
    fields_array = fields.get<array>();

    // Save the datasets to the datasets array
    value temp = get_data.get("dataSets");
    data_sets = temp.get<array>();

    // Save the media objects to the media objects array
    value temp2 = get_data.get("mediaObjects");
    media_objects = temp2.get<array>();

    // Save the owner info.
    value temp3 = get_data.get("owner");
    owner_info = temp3.get<object>();
  }

  // Clean up cURL and close the memfile
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  memfclose(json_file);

  // If we have both a title and project ID, we can grab the dataset ID from the get data.
  return true;
}


// Calls the get_datasets function, returns a vector of the data
// Must be given a valid iSENSE dataset name & field name
// Must have a project ID set.
vector<string> iSENSE::get_dataset(string dataset_name, string field_name)
{
  vector<string> vector_data;

  // Make sure a valid project ID has been set
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return vector_data;
  }

  // First call get_datasets_and_mediaobjects() and see if that is sucessful.
  if(!get_datasets_and_mediaobjects())
  {
    // We failed. Let the user know, and return an empty map.
    cerr << "\n\nFailed to get datasets.\n";
    return vector_data;
  }

  // If we get here, we can try and find the field name, and return a vector of data for that
  // field name.
  array::iterator it, the_begin, the_end;

  the_begin = data_sets.begin();
  the_end = data_sets.end();

  // Check and see if the data_sets array is empty
  if(the_begin == the_end)
  {
    // Print an error and quit, we can't make a vector of project titles to return
    // if the JSON array ended up empty. Probably wasn't any projects with that search term.
    cerr << "\nDatasets array is empty.\n";

    return vector_data;  // this is an empty vector
  }

  string dataset_ID = get_Dataset_ID(dataset_name);
  string field_ID = get_Field_ID(field_name);

  // If either dataset ID or field ID threw an error, quit.
  if(dataset_ID == GET_ERROR || field_ID == GET_ERROR)
  {
    cerr << "\n\nUnable to return a vector of data.\n";
    cerr << "Either the dataset / field names are incorrect, \n";
    cerr << "Or the project ID is wrong.\n";

    return vector_data;
  }

  // If we make here, we can make a vector containing data points and return that to the user.

  /*
   * Note on why there's 3 for loops:
   * First for loop goes through all datasets in the project and looks for one with the dataset
   * name we found above.
   * At that point, we have the object containing all the info about that dataset.
   * The second for loop then goes through the info object and looks for the array
   * of data points for this dataset.
   * The final for loop goes through this array and grabs all the data points with the above
   * field ID (field name) and stores these data points into a vector, which is then
   * returned to the user.
   *
   */

  for(it = data_sets.begin(); it != data_sets.end(); it++)    // This outer for loop is
  {                                     // for going through all datasets in the project
    // Get the current object
    object obj = it->get<object>();

    // Get that object's dataset ID to compare against
    string id = obj["id"].to_str();

    // If we have the right object, sweet!
    if(id == dataset_ID)  // We can now start looking for the data array
    {
      // obtain a const reference to the map (as seen on picoJSON github page)
      const object& cur_obj = it->get<object>();

      // This basically lets us iterate through the current object
      for (object::const_iterator i = cur_obj.begin(); i != cur_obj.end(); i++)
      {
        // When we get here, we've found the data array! WOO HOO!
        if(i->first == "data")
        {
          // Now we need one final for loop to go through the array,
          // and push_back just the data points for our field name
          // (using the field ID we found above)
          array dataset_list = i->second.get<array>();

          for(array::iterator iter = dataset_list.begin(); iter != dataset_list.end(); iter++)
          {
            // We make some tmp objects for getting datapoints, since the dataset
            // array for each dataset stores objects for each data point
            object tmp_obj = iter->get<object>();
            string data_pt = tmp_obj[field_ID].to_str();
            vector_data.push_back(data_pt);
          }

          // When we get here, we're finished. Finally return the vector of
          // data for the given field name.
          return vector_data;
        }
      }
    }
  }

  // Note: should we not find the data we're looking for, this vector should be empty!
  // Users should check the vector to make sure it is not empty & look for any error
  // messages that are printed.
  // Also note - if we get here, then we failed somewhere above since we should have
  // returned a vector of data points.
  cerr << "\n\nFailed to get data points. Check the following & make sure they are correct:\n";
  cerr << "field name, dataset name, project ID\n";
  cerr << "(picoJSON may also have failed at some point.)\n";

  return vector_data;
}


std::string iSENSE::get_Field_ID(string field_name)
{
  // Grab all the fields using an iterator.
  // Similar to printing them all out below in the debug function.
  array::iterator it;

  // Check and see if the fields object is empty
  if(fields.is<picojson::null>() == true)
  {
    // Print an error and quit, we can't do anything if the field array wasn't set up correctly.
    cerr << "\nError - field array wasn't set up.";
    cerr << "Have you pulled the fields off iSENSE?\n";
    return GET_ERROR;
  }

  // We made an iterator above, that will let us run through the fields
  for(it = fields_array.begin(); it != fields_array.end(); it++)
  {
    // Get the current object
    object obj = it->get<object>();

    // Grab the field ID and save it in a string/
    string field_ID = obj["id"].to_str();

    // Grab the field name
    string name = obj["name"].get<string>();

    // See if we can find the field name in this project
    if(name == field_name)
    {
      return field_ID;
    }
  }

  return GET_ERROR;
}


std::string iSENSE::get_Dataset_ID(string dataset_name)
{
  // Now compare the dataset name that the user provided with datasets in the project.
  // Use an iterator to go through all the datasets
  array::iterator it;

  // We made an iterator above, that will let us run through the fields
  for(it = data_sets.begin(); it != data_sets.end(); it++)
  {
    // Get the current object
    object obj = it->get<object>();

    // Grab the dataset ID and save it in a string
    string ID = obj["id"].to_str();

    // Grab the dataset name
    string name = obj["name"].get<string>();

    // Compare this current name against the dataset name that was passed into this method.
    if(name == dataset_name)
    {
      // We found the name, so return the dataset ID
      return ID;
    }
  }

  // If we get here, we failed to find the dataset name in this project.
  return GET_ERROR;
}


// Call this function to POST data to rSENSE
bool iSENSE::post_json_key()
{
  /*
   *  These first couple of if statements perform some basic error checking, such as
   *  whether or not all the required fields have been set up.
   */

  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return false;
  }

  // Check that a title and contributor key has been set.
  if(title == "title" || title.empty())
  {
    cerr << "\nError - please set a project title!\n";
    return false;
  }

  if(contributor_key.empty())
  {
    cerr << "\nErrror - please set a contributor key!\n";
    return false;
  }

  // If a label wasn't set, automatically set it to "cURL"
  if(contributor_label == "label" || contributor_label.empty())
  {
    contributor_label = "cURL";
  }

  // Make sure the map actually has stuff pushed to it.
  if(map_data.empty())
  {
    cerr << "\nMap of keys/data is empty. You should push some data back to this object.\n";
    return false;
  }

  // Should make sure each vector is not empty as well, since I had issues uploading
  // if any ONE vector was empty. rSENSE complained about the nil class.

  upload_URL = devURL + "/projects/" + project_ID + "/jsonDataUpload";

  // Call the POST function, give it type 1 since this is a upload JSON by contributor key.
  int http_code = post_data_function(1);

  /*
  *  The iSENSE API gives us two response codes to check against:
  *  Success: 200 OK (iSENSE handled the request fine)
  *  Failure: 401 Unauthorized (email / password or contributor key was not valid)
  *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
  *           but there was an issue with the request's formatting.
  *           Something in the formatting caused iSENSE to fail.)
  */

  if(http_code == 200)
  {
    cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    cout << "HTTP Response Code was: " << http_code << endl;
    cout << "The URL to your project is: " << dev_baseURL << "/projects/" << project_ID << endl;
    return true;
  }
  else
  {
    cerr << "\n\nPOST request **failed**\n";
    cerr << "HTTP Response Code was: " << http_code << endl;

    if(http_code == 401)
    {
      cerr << "\n\nTry checking to make sure your contributor key is valid\n";
      cerr << "for the project you are trying to contribute to.\n";
    }
    if(http_code == 404)
    {
      cerr << "\n\nUnable to find that project ID.\n";
    }
    if(http_code == 422)
    {
      cerr << "\n\nSomething went wrong with iSENSE.\n";
      cerr << "Try formatting your data differently,\n";
      cerr << "using an email & password instead of a contributor key,\n";
      cerr << "or asking for help from others. You can also try running the\n";
      cerr << "the program with the \"debug\" method enabled, by typing: \n";
      cerr << "object_name.debug()\n";
      cerr << "This will output a ton of data to the console and may help you in\n";
      cerr << "debugging your program.\n";
    }
    if(http_code == CURL_ERROR)
    {
      cerr << "\n\nCurl failed for some unknown reason.\n";
      cerr << "Make sure you've installed curl / libcurl, and have the \n";
      cerr << "picojson header file as well.\n";
    }
  }

  return false;
}


/*    Append to a dataset using its dataset_ID.
 *    The dataset ID can be found on iSENSE by going to a project and clicking on
 *    a dataset.
 *    In the future, uploading JSON will return the dataset ID for this function
 *    (assuming iSENSE allows that)
 */
bool iSENSE::append_key_byID(string dataset_ID)
{
  /*
   *  These first couple of if statements perform some basic error checking, such as
   *  whether or not all the required fields have been set up.
   */

  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return false;
  }

  // Check that a title and contributor key has been set.
  if(title == "title" || title.empty())
  {
    cerr << "\nError - please set a project title!\n";
    return false;
  }

  if(contributor_key.empty())
  {
    cerr << "\nErrror - please set a contributor key!\n";
    return false;
  }

  // If a label wasn't set, automatically set it to "cURL"
  if(contributor_label == "label" || contributor_label.empty())
  {
    contributor_label = "cURL";
  }

  // Make sure the map actually has stuff pushed to it.
  if(map_data.empty())
  {
    cerr << "\nMap of keys/data is empty. You should push some data back to this object.\n";
    return false;
  }

  // Set the dataset_ID
  set_dataset_ID(dataset_ID);

  // Set the append API URL
  upload_URL = devURL + "/data_sets/append";

  // Call the POST function, give it type 2 since this is an append by contributor key.
  int http_code = post_data_function(2);

   /*
    *  The iSENSE API gives us two response codes to check against:
    *  Success: 200 OK (iSENSE handled the request fine)
    *  Failure: 401 Unauthorized (email / password or contributor key was not valid)
    *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
    *           but there was an issue with the request's formatting.
    *           Something in the formatting caused iSENSE to fail.)
    */

  if(http_code == 200)
  {
    cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    cout << "HTTP Response Code was: " << http_code << endl;
    cout << "The URL to your project is: " << dev_baseURL << "/projects/" << project_ID << endl;
    return true;
  }
  else
  {
    cerr << "\n\nPOST request **failed**\n";
    cerr << "HTTP Response Code was: " << http_code << endl;

    if(http_code == 401)
    {
      cerr << "Try checking to make sure your contributor key is valid\n";
      cerr << "for the project you are trying to contribute to.\n";
    }
    if(http_code == 422)
    {
      cerr << "Something went wrong with iSENSE.\n";
      cerr << "Try formatting your data differently,\n";
      cerr << "using an email & password instead of a contributor key,\n";
      cerr << "or asking for help from others. You can also try running the\n";
      cerr << "the program with the \"debug\" method enabled, by typing: \n";
      cerr << "object_name.debug()\n";
      cerr << "This will output a ton of data to the console and may help you in\n";
      cerr << "debugging your program.\n";
    }
    if(http_code == CURL_ERROR)
    {
      cerr << "\n\nCurl failed for some unknown reason.\n";
      cerr << "Make sure you've installed curl / libcurl, and have the \n";
      cerr << "picojson header file as well.\n";
    }
  }

  return false;
}


/*
 *    Appends to a dataset using its dataset name, which can be used to find a dataset ID
 *    We can find the dataset ID by comparing against all the datasets in a given project
 *    until we find the dataset with the given name.
 *
 */
bool iSENSE::append_key_byName(string dataset_name)
{
  /*
   *  These first couple of if statements perform some basic error checking, such as
   *  whether or not all the required fields have been set up.
   */

  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return false;
  }

  // Check that a title and contributor key has been set.
  if(title == "title" || title.empty())
  {
    cerr << "\nError - please set a project title!\n";
    return false;
  }

  if(contributor_key.empty())
  {
    cerr << "\nErrror - please set a contributor key!\n";
    return false;
  }

  // If a label wasn't set, automatically set it to "cURL"
  if(contributor_label == "label" || contributor_label.empty())
  {
    contributor_label = "cURL";
  }

  // Make sure the map actually has stuff pushed to it.
  if(map_data.empty())
  {
    cerr << "\nMap of keys/data is empty. You should push some data back to this object.\n";
    return false;
  }

  // We can now find the dataset ID by comparing against all the datasets in this project.
  // First pull down the datasets
  get_datasets_and_mediaobjects();

  // Call the get_dataset_ID function
  string dataset_ID = get_Dataset_ID(dataset_name);

  // If we didn't get any errors, call the append by ID function.
  if(dataset_ID != GET_ERROR)
  {
    append_key_byID(dataset_ID);
    return true;
  }

  // If we got here, we failed to find that dataset name in the current project.
  cout << "Failed to find the dataset name in project # " << project_ID << endl;
  cout << "Make sure to type the exact name, as it appears on iSENSE. \n";
  return false;
}


// Post using a email / password
bool iSENSE::post_json_email()
{
  /*
   *        These first couple of if statements perform some basic error checking, such as
   *        whether or not all the required fields have been set up.
   */

  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return false;
  }

  // Check that a title and contributor key has been set.
  if(title == "title" || title.empty())
  {
    cerr << "\nError - please set a project title!\n";
    return false;
  }

  if(email == "email" || email.empty())
  {
    cerr << "\nErrror - please set an email address!\n";
    return false;
  }

  if(password == "password" || password.empty())
  {
    cerr << "\nErrror - please set a password!\n";
    return false;
  }

  // If a label wasn't set, automatically set it to "cURL"
  if(contributor_label == "label" || contributor_label.empty())
  {
    contributor_label = "cURL";
  }

  // Make sure the map actually has stuff pushed to it.
  if(map_data.empty())
  {
    cerr << "\nMap of keys/data is empty. You should push some data back to this object.\n";
    return false;
  }

  // Should make sure each vector is not empty as well, since I had issues uploading
  // if any ONE vector was empty. rSENSE complained about the nil class.

  // Make sure to set the upload URL!
  upload_URL = devURL + "/projects/" + project_ID + "/jsonDataUpload";

  // Call the POST function, give it type 3 since this is upload JSON by email & password.
  int http_code = post_data_function(3);

  /*
    *  The iSENSE API gives us two response codes to check against:
    *  Success: 200 OK (iSENSE handled the request fine)
    *  Failure: 401 Unauthorized (email / password or contributor key was not valid)
    *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
    *               but there was an issue with the upload for some reason.)
    *               the request's formatting. Something in the formatting caused iSENSE to fail.)
    */

  if(http_code == 200)
  {
    cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    cout << "HTTP Response Code was: " << http_code << endl;
    cout << "The URL to your project is: " << dev_baseURL << "/projects/" << project_ID << endl;
    return true;
  }
  else
  {
    cerr << "\nPOST request **failed**\n";
    cerr << "HTTP Response Code was: " << http_code << endl;

    if(http_code == 401)
    {
      cerr << "Try checking to make sure your contributor key is valid\n";
      cerr << "for the project you are trying to contribute to.\n";
    }
    if(http_code == 422)
    {
      cerr << "Something went wrong with iSENSE.\n";
      cerr << "Try formatting your data differently,\n";
      cerr << "using a contributor key instead of an email/password,\n";
      cerr << "or asking for help from others. You can also try running the\n";
      cerr << "the program with the \"debug\" method enabled, by typing: \n";
      cerr << "object_name.debug()\n";
      cerr << "This will output a ton of data to the console and may help you in\n";
      cerr << "debugging your program.\n";
    }
    if(http_code == CURL_ERROR)
    {
      cerr << "\n\nCurl failed for some unknown reason.\n";
      cerr << "Make sure you've installed curl / libcurl, and have the \n";
      cerr << "picojson header file as well.\n";
    }
  }

  return false;
}


// Post append using email and password
bool iSENSE::append_email_byID(string dataset_ID)
{
  /*
   *        These first couple of if statements perform some basic error checking, such as
   *        whether or not all the required fields have been set up.
   */

  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return false;
  }

  // Check that a title and contributor key has been set.
  if(title == "title" || title.empty())
  {
    cerr << "\nError - please set a project title!\n";
    return false;
  }

  if(email == "email" || email.empty())
  {
    cerr << "\nErrror - please set an email address!\n";
    return false;
  }

  if(password == "password" || password.empty())
  {
    cerr << "\nErrror - please set a password!\n";
    return false;
  }

  // Make sure the map actually has stuff pushed to it.
  if(map_data.empty())
  {
    cerr << "\nMap of keys/data is empty. You should push some data back to this object.\n";
    return false;
  }

  // Set the dataset_ID
  set_dataset_ID(dataset_ID);

  // Change the upload URL to append rather than create a new dataset.
  upload_URL = devURL + "/data_sets/append";

  // Call the POST function, give it type 4 since this is append JSON by email & password.
  int http_code = post_data_function(4);

  /*
    *  The iSENSE API gives us two response codes to check against:
    *  Success: 200 OK (iSENSE handled the request fine)
    *  Failure: 401 Unauthorized (email / password or contributor key was not valid)
    *  Failure: 422 Unprocessable Entity (email or contributor key was fine,
    *               but there was an issue with the upload for some reason.)
    *               the request's formatting. Something in the formatting caused iSENSE to fail.)
    */

  if(http_code == 200)
  {
    cout << "\n\nPOST request successfully sent off to iSENSE!\n";
    cout << "HTTP Response Code was: " << http_code << endl;
    cout << "The URL to your project is: " << dev_baseURL << "/projects/" << project_ID << endl;
    return true;
  }
  else
  {
    cerr << "\nPOST request **failed**\n";
    cerr << "HTTP Response Code was: " << http_code << endl;

    if(http_code == 401)
    {
      cerr << "Try checking to make sure your contributor key is valid\n";
      cerr << "for the project you are trying to contribute to.\n";
    }
    if(http_code == 422)
    {
      cerr << "Something went wrong with iSENSE.\n";
      cerr << "Try formatting your data differently,\n";
      cerr << "using a contributor key instead of an email/password,\n";
      cerr << "or asking for help from others. You can also try running the\n";
      cerr << "the program with the \"debug\" method enabled, by typing: \n";
      cerr << "object_name.debug()\n";
      cerr << "This will output a ton of data to the console and may help you in\n";
      cerr << "debugging your program.\n";
    }
    if(http_code == CURL_ERROR)
    {
      cerr << "\n\nCurl failed for some unknown reason.\n";
      cerr << "Make sure you've installed curl / libcurl, and have the \n";
      cerr << "picojson header file as well.\n";
    }
  }

  return false;
}


/*
 *    Appends to a dataset using its dataset name, which can be used to find a dataset ID
 *    We can find the dataset ID by comparing against all the datasets in a given project
 *    until we find the dataset with the given name.
 *
 */
bool iSENSE::append_email_byName(string dataset_name)
{
  // Check that the project ID is set properly.
  // When the ID is set, the fields are also pulled down as well.
  if(project_ID == "empty" || project_ID.empty())
  {
    cerr << "\nError - please set a project ID!\n";
    return false;
  }

  // Check that a title and contributor key has been set.
  if(title == "title" || title.empty())
  {
    cerr << "\nError - please set a project title!\n";
    return false;
  }

  if(email == "email" || email.empty())
  {
    cerr << "\nErrror - please set an email address!\n";
    return false;
  }

  if(password == "password" || password.empty())
  {
    cerr << "\nErrror - please set a password!\n";
    return false;
  }

  // Make sure the map actually has stuff pushed to it.
  if(map_data.empty())
  {
    cerr << "\nMap of keys/data is empty. You should push some data back to this object.\n";
    return false;
  }

  // We can now find the dataset ID by comparing against all the datasets in this project.
  // First pull down the datasets
  get_datasets_and_mediaobjects();

  // Call the get_dataset_ID function
  string dataset_ID = get_Dataset_ID(dataset_name);

  // If we didn't get any errors, call the append by ID function.
  if(dataset_ID != GET_ERROR)
  {
    append_key_byID(dataset_ID);
    return true;
  }

  // If we got here, we failed to find that dataset name in the current project.
  cerr << "Failed to find the dataset name in project # " << project_ID << endl;
  cerr << "Make sure to type the exact name, as it appears on iSENSE. \n";
  return false;
}


// This function is called by the JSON upload function
// It formats the upload string
// Users should not have to call this function - API methods will,
// and will pass an int value indicating which API method they are using.
void iSENSE::format_upload_string(int key)
{
  // Add the title + the correct formatting
  upload_data["title"] = value(title);

  // This is now a switch. Any future API methods can be added here.
  switch(key)
  {
    case 1:
      upload_data["contribution_key"] = value(contributor_key);
      upload_data["contributor_name"] = value(contributor_label);
      break;

    case 2:
      upload_data["contribution_key"] = value(contributor_key);
      upload_data["id"] = value(dataset_ID);
      break;

    case 3:
      upload_data["email"] = value(email);
      upload_data["password"] = value(password);
      break;

    case 4:
      upload_data["email"] = value(email);
      upload_data["password"] = value(password);
      upload_data["id"] = value(dataset_ID);
      break;
  }

  // Add each field, with its field ID and an array of all the data in its vector.

  // Grab all the fields using an iterator.
  // Similar to printing them all out below in the debug function.
  array::iterator it;

  // Pointer to one of the vectors in the map
  vector<string> *vect;

  // Check and see if the fields object is empty
  if(fields.is<picojson::null>() == true)
  {
    // Print an error and quit, we can't do anything if the field array wasn't set up correctly.
    cerr << "\nError - field array wasn't set up.";
    cerr << "Have you pulled the fields off iSENSE?\n";
    return;
  }

  // We made an iterator above, that will let us run through the fields
  for(it = fields_array.begin(); it != fields_array.end(); it++)
  {
    // Get the current object
    object obj = it->get<object>();

    // Grab the field ID and save it in a string/
    string field_ID = obj["id"].to_str();

    // Grab the field name
    string name = obj["name"].get<string>();

    // Now add all the data in that field's vector (inside the map)
    // to the fields_array object.
    vect = &map_data[name];
    format_data(vect, it, field_ID);
  }

  // Once we've made the field_data object, we can
  // add the field_data object to the upload_data object
  upload_data["data"] = value(fields_data);
}


// This makes the switch above shorter,
// since I reuse this code for all 5 types of data.
void iSENSE::format_data(vector<string> *vect, array::iterator it, string field_ID)
{
  vector<string>::iterator x;    // For going through the vector
  value::array data;             // Using a picojson::value::array,
  // basically a vector but represents a json array.

  // First we push all the vector data into a json array.
  for(x = vect -> begin(); x < vect -> end(); x++)
  {
    data.push_back(value(*x));
  }

  // Now we push the json array to the upload_data object.
  fields_data[field_ID] = value(data);
}


/*    This function is called by all of the POST functions.
 *    It must be given a parameter, an integer "type", which determines
 *    which way the JSON should be formatted in the format_upload_string function.
 *
 *    Function returns an HTTP response code, like "200", "404", "503", etc.
 */
int iSENSE::post_data_function(int type)
{
  // Upload_URL must have already been set. Otherwise the POST request will fail
  // unexpectedly.
  if(upload_URL.empty() || upload_URL == "URL")
  {
    cout << "\n\nPlease set a valid upload URL.\n";
    return -1;
  }

  // Format the data to be uploaded. Call another function to format this.
  format_upload_string(type);

  /*  Once we get the data formatted, we can try to POST to rSENSE
   *    The below code uses cURL. It
   *    1. Sets the headers, so iSENSE knows we are sending it JSON
   *    2. Does some curl init stuff that makes the magic happen.
   *    3. cURL sends off the request, we can grab the return code to see if cURL failed.
   *       Also check the curl verbose debug to see why something failed.
   *    4. We also get the HTTP status code so we know if iSENSE handled the request or not. */

  // CURL object and response code.
  CURL *curl = curl_easy_init();          // cURL object
  long http_code;                         // HTTP status code

  // In windows, this will init the winsock stuff
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Set the headers to JSON, make sure to use UTF-8
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Accept-Charset: utf-8");
  headers = curl_slist_append(headers, "charsets: utf-8");
  headers = curl_slist_append(headers, "Content-Type: application/json");

  // get a curl handle
  curl = curl_easy_init();

  if(curl)
  {
    // Set the URL that we will be using for our POST.
    curl_easy_setopt(curl, CURLOPT_URL, upload_URL.c_str());

    // This is necessary! As I had issues with only 1 byte being sent off to iSENSE
    // unless I made sure to make a string out of the upload_data picojson object,
    // then with that string you can call c_str() on it below.
    string upload_real = (value(upload_data).serialize());

    // POST data. Upload will be the string with all the data.
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, upload_real.c_str());

    // JSON Headers
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Verbose debug output - turn this on if you are having problems. It will spit out
    // a ton of information, such as bytes sent off, headers/access/etc.
    // Useful to see if you formatted the data right.
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    cout << "\nrSENSE response: \n";

    // Perform the request, res will get the return code
    curl_easy_perform(curl);

    // This will put the HTTP response code into the "http_code" variable.
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    // Clean up curl.
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // Return the HTTP code we get from curl.
    return http_code;
  }

  // If curl fails for some reason, return CURL_ERROR (-1).
  return CURL_ERROR;
}


// Call this function to dump all the data in the given object.
void iSENSE::debug()
{
  cout << "\nProject Title: " << title << endl;
  cout << "Project ID: " << project_ID << endl;
  cout << "Dataset ID: " << dataset_ID << endl;
  cout << "Contributor Key: " << contributor_key << endl;
  cout << "Contributor Label: " << contributor_label << endl;
  cout << "Email Address: " << email << endl;
  cout << "Password: " << password << endl;
  cout << "Upload URL: " << upload_URL << endl;
  cout << "GET URL: " << get_URL << endl;
  cout << "GET User URL: " << get_UserURL << "\n\n";
  cout << "Upload string (picojson object): \n" << value(upload_data).serialize() << "\n\n";
  cout << "Upload Fields Data (picojson object) \n" << value(fields_data).serialize() << "\n\n";
  cout << "GET Data (picojson value): \n" << get_data.serialize() << "\n\n";
  cout << "GET Field Data (picojson value): \n" << fields.serialize() << "\n\n";
  cout << "GET Fields array (picojson array): \n" << value(fields_array).serialize() << "\n\n";

  // This part may get huge depending on the project!
  // May want a confirm if the user actually wants to display these.
  cout << "Datasets (picojson array): " << value(data_sets).serialize() << "\n\n";
  cout << "Media objects (picojson array): " << value(media_objects).serialize() << "\n\n";
  cout << "Owner info (picojson object): " << value(owner_info).serialize() << "\n\n";

  cout << "Map data: \n\n";

  // These for loops will dump all the data in the map.
  // Good for debugging.
  for(map<string, vector<string>>::iterator it = map_data.begin();
      it != map_data.end(); it++)
      {
        cout << it->first << " ";

        for(vector<string>::iterator vect = (it->second).begin();
            vect != (it->second).end(); vect++)
            {
              cout << *vect << " ";
            }

        cout << "\n";
      }
}


//******************************************************************************
// These are needed for picojson & libcURL.
// Declared in memfile.h but defined below.
MEMFILE*  memfopen()
{
  MEMFILE* mf = (MEMFILE*) malloc(sizeof(MEMFILE));
  mf->data = NULL;
  mf->size = 0;
  return mf;
}


void memfclose(MEMFILE* mf)
{
  // Double check to make sure that mf exists.
  if(mf == NULL)
  {
    return;
  }

  // OK to free the char array
  if (mf != NULL && mf->data)
  {
    free(mf->data);
  }

  // And OK to free the structure
  free(mf);
}


// Simple function only used by the get_check_user function to
// suppress curl's output to the screen.
size_t suppress_output(char* ptr, size_t size, size_t nmemb, void* stream)
{
  return size * nmemb;
}


size_t memfwrite(char* ptr, size_t size, size_t nmemb, void* stream)
{
  MEMFILE* mf = (MEMFILE*) stream;
  int block = size * nmemb;

  if (!mf->data)
  {
    mf->data = (char*) malloc(block);
  }
  else {
    mf->data = (char*) realloc(mf->data, mf->size + block);
  }

  if (mf->data)
  {
    memcpy(mf->data + mf->size, ptr, block);
    mf->size += block;
  }

  return block;
}


char* memfstrdup(MEMFILE* mf)
{
  char* buf = (char*)malloc(mf->size + 1);
  memcpy(buf, mf->data, mf->size);
  buf[mf->size] = 0;

  return buf;
}