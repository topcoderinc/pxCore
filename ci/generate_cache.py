import sys, getopt
import json
import requests
from pprint import pprint

def handleRequest(argv):
	"""Function to read user request, form http message and send it"""
	try:
		opts, args = getopt.getopt(argv,"",["help","repo=","org=","user=","apitoken="])
	except getopt.GetoptError:
		print 'usage: python generate_cache.py --repo=<repo> --org=<org> --user=<user> --apitoken=<apitoken>'
		exit (1)
	
	#initialize input values
	repo_name="pxCore";
	user_name="";
	api_token="";

	#read input values
	for opt, arg in opts:
		if opt == "--help":
			print 'python generate_cache.py --repo=<repo> --org=<org> --user=<user> --apitoken=<apitoken>'
			exit (1)
		elif len(arg) == 0:
			print "argument cannot be empty for option", opt
			exit (1)
		elif opt == "--repo":
			repo_name=arg;
		elif opt == "--user":
			user_name=arg;
		elif opt == "--apitoken":
			api_token=arg;
		elif opt == "--org":
			org_name=arg;

	if user_name=="" or api_token=="":
		print 'usage: python generate_cache.py --repo=<repo> --org=<org> --user=<user> --apitoken=<apitoken>'
		exit (1)

	with open('pxscene_generate_cache.json') as data_file:
	    data = json.load(data_file)
	
	string = json.dumps(data);
	
	#print the request json data
	pprint(string);
	
	tokendata = "token ";
	tokendata += str(api_token);
	
	#populate http header
	headers = {}
	headers["Content-Type" ] = "application/json";
	headers["Accept" ] = "application/json";
	headers["Travis-API-Version"] = "3";
	headers["Authorization"] = tokendata;
	
	url = "https://api.travis-ci.org/repo/" + str(org_name) + "%2F" + str(repo_name) + "/requests";
	
	#send http request
	response = requests.post(url, headers=headers, data=string)
	print response.content

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print 'usage: python generate_cache.py --repo=<repo> --user=<user> --apitoken=<apitoken>'
		exit (1)
	handleRequest(sys.argv[1:])
