/* Filename: GPXParser.c
 * Version: A2 - Preliminary Harness Pass.
 * UoG: 1096689
 * Student Name: Greg Shalay 
 * Description: This file will parse a given GPX file and build a GPXDoc object that resembles the dynamic structure of the file provided. There are also some statistic functions that report the number of various data
 *              objects that exist in the file.
 * 
 * Citations: This source code uses some of the framework code from the example tree1.c (notably the nested for loop for XML tree traversal) provided by http://www.xmlsoft.org/examples/tree1.c 
 *            It also utilizes some libxml2 provided functions to parse specific data (documentation accessible here: http://xmlsoft.org/html/libxml-tree.html) 
 *            Of the specific functions, the one most notably used was the xmlNodeGetContent function whose documentation is available via http://xmlsoft.org/html/libxml-tree.html#xmlNodeGetContent. 
 *            Parsing a GPXDoc to a XmlDoc (libxml2 tree) is based off of the example found here: http://xmlsoft.org/examples/tree2.c.
 */

#include "GPXParser.h"
#include <stdbool.h>

#define EQUAL_STRINGS 0
#define NO_ELEMENTS 0
#define MAX_READ_CHARS 256
#define DOUBLE_CHARS 325
#define GPXDATA_SIZE 1000
#define JSON_LIST_STR_MAX_LENGTH 6000
#define JSON_GPX_STR_MAX_LENGTH 9000
#define MAX_NAME_LENGTH 1250
#define HALF_CIRCLE_DEGREES 180
#define MIN_LOOP_WPTS 4
#define DEFAULT_DELTA 10
#define JSON_NAME_LEN 257
#define JSON_WPT_STR_LEN 600
#define JSON_STR_LEN 1000
#define FILE_JSON_STR_LEN 10000

#define MAX_LATITUDE 90.000000
#define MIN_LATITUDE -90.000000
#define MAX_LONGITUDE 180.000000
#define MIN_LONGITUDE -180.000000

#define SENTINEL_LAT_LON -200.000000 // Base invalid value. If a lat or lon isn't properly set, then this value will indicate so.
#define SENTINEL_VERSION -1.0 // Same purpose as the sentinel value above, but for the gpx version.

#define GPX "gpx"
#define TRK "trk"
#define TRKSEG "trkseg"
#define TRKPT "trkpt"
#define RTEPT "rtept"
#define WPT "wpt"
#define ELE "ele"
#define RTE "rte"
#define VERSION "version"
#define CREATOR "creator"
#define TEXT "text"
#define LAT "lat"
#define LON "lon"
#define NAME "name"
#define DEFAULT_NAMESPACE "http://www.topografix.com/GPX/1/1"


bool parseFail = false;

/* **************************************************************************CONSTRUCTORS**************************************************************************************** */

GPXdoc * buildGPXdoc(GPXdoc * gpx, char * schemaLocation, char * version, char * creator){
  char * endPtr;

  if(gpx == NULL || schemaLocation == NULL || version == NULL || creator == NULL){
    free(gpx);
    return NULL;
  }

  int strMemLen = strlen(creator) + 2;

  if(strcmp(version, "\0") == EQUAL_STRINGS){
    gpx->version = -1.0;
  }
  else{
    gpx->version = strtod(version, &endPtr);
  }

  gpx->creator = (char *) malloc(sizeof(char) * strMemLen);

  if(gpx->creator == NULL){
    free(gpx->creator);
    free(gpx);
    return NULL;
  }

  strcpy(gpx->creator, creator);
  strcpy(gpx->namespace, schemaLocation);

  gpx->waypoints = initializeList(waypointToString, deleteWaypoint, compareWaypoints);
  gpx->routes = initializeList(routeToString, deleteRoute, compareRoutes);
  gpx->tracks = initializeList(trackToString, deleteTrack, compareTracks);

  if(gpx->waypoints == NULL || gpx->routes == NULL || gpx->tracks == NULL){
    freeList(gpx->waypoints);
    freeList(gpx->routes);
    freeList(gpx->tracks);
    free(gpx);
    return NULL;
  }

  return gpx;
}

Track * buildTrack(Track * track, char * name){
  int strMemLen = 0;
  track = (Track *) malloc(sizeof(Track));
  track->name = (char *) malloc(sizeof(char));

  if(track == NULL || track->name == NULL || name == NULL){
    free(track->name);
    free(track);
    return NULL;
  }
  else{
    strMemLen = strlen(name) + 2;
    strcpy(track->name, "\0");
    track->segments = initializeList(trackSegmentToString, deleteTrackSegment, compareTrackSegments);
    track->otherData = initializeList(gpxDataToString, deleteGpxData, compareGpxData);

    if(track->segments == NULL || track->otherData == NULL){
      freeList(track->segments);
      freeList(track->otherData);
      free(track->name);
      free(track);
      return NULL;      
    }

  } 

  if(!(strcmp(name, "\0") == EQUAL_STRINGS)){
    free(track->name);
    track->name = (char *) malloc(sizeof(char) * strMemLen); 

    if(track->name == NULL){
      freeList(track->segments);
      freeList(track->otherData);
      free(track->name);
      free(track);
      return NULL;
    }

    strcpy(track->name, name);
  }
  
  return track;
}

Route * buildRoute(Route * route, char * name){
  int strMemLen = 1;
  route = (Route *) malloc(sizeof(Route));
  route->name = (char *) malloc(sizeof(char));

  if(route == NULL || route->name == NULL || name == NULL){
    free(route->name);
    free(route);
    return NULL;
  }
  else{
    strMemLen = strlen(name) + 2;
    strcpy(route->name, "\0");
    route->waypoints = initializeList(waypointToString, deleteWaypoint, compareWaypoints);
    route->otherData = initializeList(gpxDataToString, deleteGpxData, compareGpxData);

    if(route->waypoints == NULL || route->otherData == NULL){
      freeList(route->waypoints);
      freeList(route->otherData);
      free(route->name);
      free(route);
      return NULL;
    }

  }
  
  if(!(strcmp(name, "\0") == EQUAL_STRINGS)){
    free(route->name);
    route->name = (char *) malloc(sizeof(char) * strMemLen);

    if(route->name == NULL){
      free(route->name);
      free(route);
      return NULL;
    }

    strcpy(route->name, name);
  }
  
  return route;
}

Waypoint * buildWaypoint(Waypoint * waypoint, char * name, char * longitude, char * latitude){
  char * endPtr;
  int strMemLen = 0;

  waypoint = (Waypoint *) malloc(sizeof(Waypoint));
  waypoint->name = (char *) malloc(sizeof(char) + 2);

  if(waypoint == NULL || waypoint->name == NULL || name == NULL || longitude == NULL || latitude == NULL){
    free(waypoint->name);
    free(waypoint);
    return NULL;
  }
  else{
    strMemLen = (sizeof(char) * strlen(name)) + 10;
    strcpy(waypoint->name, "\0");
    waypoint->longitude = SENTINEL_LAT_LON;
    waypoint->latitude = SENTINEL_LAT_LON;
    waypoint->otherData = initializeList(gpxDataToString, deleteGpxData, compareGpxData);

    if(waypoint->otherData == NULL){
      freeList(waypoint->otherData);
      free(waypoint->name);
      free(waypoint);
      return NULL;
    }
  }

  if(!(strcmp(name, "\0") == EQUAL_STRINGS)){
    waypoint->name = (char *) realloc(waypoint->name, (sizeof(char) * strMemLen));
  }
  
  if(waypoint->name == NULL){    
    free(waypoint->name);
    free(waypoint);
    return NULL;
  }
  else{
    strcpy(waypoint->name, name);
    if(!(strcmp(longitude, "\0") == EQUAL_STRINGS)){
      waypoint->longitude = strtod(longitude, &endPtr);  
    }
    if(!(strcmp(longitude, "\0") == EQUAL_STRINGS)){
      waypoint->latitude = strtod(latitude, &endPtr);
    }
  }

  return waypoint;
}

GPXData * buildGPXData(GPXData * gpxData, char * name, char * value){
  gpxData = (GPXData *) malloc(sizeof(GPXData) + (2 * sizeof(char))); // For the empty flexible array member.
  int strMemLen = 1;

  if(gpxData == NULL || name == NULL || value == NULL || strcmp(value, "\0") == EQUAL_STRINGS){
    free(gpxData);
    return NULL;
  }
  else{
    strMemLen = (sizeof(char) * strlen(value)) + 5;
    strcpy(gpxData->name, "\0");
    strcpy(gpxData->value, "\0");
  }

  if(!(strcmp(value, "\0") == EQUAL_STRINGS)){
    gpxData = (GPXData *) realloc(gpxData, sizeof(GPXData) + strMemLen);
  }

  if(gpxData == NULL){
    free(gpxData);
    return NULL;
  }

  strcpy(gpxData->name, name);
  strcpy(gpxData->value, value);

  return gpxData;
}

TrackSegment * buildTrackSegment(TrackSegment * trackSegment){
  trackSegment = (TrackSegment *) malloc(sizeof(TrackSegment));

  if(trackSegment == NULL){
    free(trackSegment);
    return NULL;
  }

  trackSegment->waypoints = initializeList(waypointToString, deleteWaypoint, compareWaypoints);

  if(trackSegment->waypoints == NULL){
    free(trackSegment->waypoints);
    free(trackSegment);
    return NULL;
  }

  return trackSegment;
}

GPXdoc * buildObjects(xmlNode * a_node, GPXdoc * gpx){
  xmlNode * cur_node = NULL;

  Track * track;
  TrackSegment * trackSegment;
  Waypoint * waypoint;
  Route * route;
  GPXData * gpxData;

  char * gpxSchema = "\0";
  char * gpxVersion = "\0";
  char * gpxCreator = "\0";

  char trkName[MAX_NAME_LENGTH] = "\0";
  char wptName[MAX_NAME_LENGTH] = "\0";
  char rteName[MAX_NAME_LENGTH] = "\0";
  
  char * wptLat = "\0";
  char * wptLon = "\0";

  char * gpxDataName = "\0";
  char * gpxDataValue = "\0";

  for (cur_node = a_node; cur_node != NULL; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE){
      if(strcmp((char *) cur_node->name, RTE) == EQUAL_STRINGS){
        xmlNode * child = cur_node->children->next;
        xmlChar * content = xmlNodeGetContent(child);
        
        if(cur_node->children != NULL){
          while(child != NULL && child != child->last){
            if(strcmp((char *) child->name, NAME) == EQUAL_STRINGS){
              strcpy(rteName, (char *) content);
            }

            child = child->next;
          }
                        
          xmlFree(content);
        } 
      }

      if(strcmp((char *) cur_node->name, TRK) == EQUAL_STRINGS){
        if(cur_node->children != NULL){
          xmlNode * child = cur_node->children->next;

          xmlChar * content = xmlNodeGetContent(child);
          while(child != NULL && child != child->last){
            if(strcmp((char *) child->name, NAME) == EQUAL_STRINGS){
              strcpy(trkName, (char *) content);
            }

            child = child->next;
          }
                        
          xmlFree(content);
        } 
      }

      else if(strcmp((char *) cur_node->name, TRKSEG) == EQUAL_STRINGS){
        if((getFromBack(gpx->tracks)) == NULL){
          track = (Track *) buildTrack(track, "\0");

          if(track == NULL){
            parseFail = true;
            return NULL;
          }

          insertBack(gpx->tracks, (void *) track); 
        }
        else{
          track = (Track *) getFromBack(gpx->tracks);
        }

        trackSegment = buildTrackSegment(trackSegment);

        if(trackSegment == NULL){
          parseFail = true;
          return NULL;
        }

        insertBack(track->segments, (void *) trackSegment);
      }

      xmlAttr *attr;
      for (attr = cur_node->properties; attr != NULL; attr = attr->next) {
        xmlNode * value = attr->children;
        char * attrName = (char *) attr->name;
        char * cont = (char *) (value->content);

        if(strcmp((char *) cur_node->name, GPX) == EQUAL_STRINGS){
          gpxSchema = (char *) cur_node->ns->href;

          if(strcmp(attrName, VERSION) == EQUAL_STRINGS){
            gpxVersion = cont;
          }
          else if(strcmp(attrName, CREATOR) == EQUAL_STRINGS){
            gpxCreator = cont;
          }
        }
        else if(strcmp((char *) cur_node->name, TRKPT) == EQUAL_STRINGS || strcmp((char *) cur_node->name, WPT) == EQUAL_STRINGS ||
                strcmp((char *) cur_node->name, RTEPT) == EQUAL_STRINGS){ 
          
          if(cur_node->children != NULL){
            xmlNode * child = cur_node->children->next;

            xmlChar * content = xmlNodeGetContent(child);
            while(child != NULL && child != child->last){
              if(strcmp((char *) child->name, NAME) == EQUAL_STRINGS){
                strcpy(wptName, (char *) content);
              }

              child = child->next;
            }
                          
            xmlFree(content);
          }          

          if(strcmp(attrName, LAT) == EQUAL_STRINGS){
            wptLat = cont;
          }
          else if(strcmp(attrName, LON) == EQUAL_STRINGS){
            wptLon = cont;
          }
        }
      }

      if(strcmp((char *) cur_node->name, GPX) == EQUAL_STRINGS){
        gpx = buildGPXdoc(gpx, gpxSchema, gpxVersion, gpxCreator);

        if(gpx == NULL){
          parseFail = true;
          return NULL;
        }
      }
      else if(strcmp((char *) cur_node->name, TRKPT) == EQUAL_STRINGS || strcmp((char *) cur_node->name, WPT) == EQUAL_STRINGS ||
              strcmp((char *) cur_node->name, RTEPT) == EQUAL_STRINGS){

        waypoint = buildWaypoint(waypoint, wptName, wptLon, wptLat);

        strcpy(wptName, "\0");

        if(waypoint == NULL){
          parseFail = true;
          return NULL;
        }

        if(cur_node->children != NULL){
          xmlNode * child = cur_node->children->next;

          while(child != NULL && child != child->last){
            xmlChar * content = xmlNodeGetContent(child);
            if(strcmp((char *) child->name, TEXT) != EQUAL_STRINGS && strcmp((char *) child->name, NAME) != EQUAL_STRINGS){
              gpxDataName = (char *) child->name;
              gpxDataValue = (char *) content;

              gpxData = buildGPXData(gpxData, gpxDataName, gpxDataValue);

              if(gpxData == NULL){
                parseFail = true;
                return NULL;
              }

              insertBack(waypoint->otherData, gpxData);
            }

            xmlFree(content);
            child = child->next;
          }
        }

        if(strcmp((char *) cur_node->name, WPT) == EQUAL_STRINGS){
          insertBack(gpx->waypoints, (void *) waypoint);  
        }
        else if(strcmp((char *) cur_node->name, TRKPT) == EQUAL_STRINGS){
          if(getFromBack(gpx->tracks) == NULL){
            track = buildTrack(track, "\0"); // Since there is no regular data to store here.

            if(track == NULL){
              parseFail = true;
              return NULL;
            }

            insertBack(gpx->tracks, (void *) track);
          }
          else{
            track = (Track *) getFromBack(gpx->tracks);
          }

          if((getFromBack(track->segments)) == NULL){
            trackSegment = buildTrackSegment(trackSegment);

            if(trackSegment == NULL){
              parseFail = true;
              return NULL;
            }

            insertBack(track->segments, (void *) trackSegment);
          }
          else{
            trackSegment = (TrackSegment *) getFromBack(track->segments);
          }
          
          insertBack(trackSegment->waypoints, (void *) waypoint);
        }
        else if(strcmp((char *) cur_node->name, RTEPT) == EQUAL_STRINGS){
          if((getFromBack(gpx->routes)) == NULL){
            route = buildRoute(route,"\0");

            if(route == NULL){
              parseFail = true;
              return NULL;
            }

            insertBack(gpx->routes, (void *) route);        
          }
          else{
            route = (Route *) getFromBack(gpx->routes);
          }

          insertBack(route->waypoints, (void *) waypoint);
        }
      }
      else if(strcmp((char *) cur_node->name, TRK) == EQUAL_STRINGS){
        track = (Track *) buildTrack(track, trkName);

        if(track == NULL){
          parseFail = true;
          return NULL;
        }

        strcpy(trkName, "\0");

        if(cur_node->children != NULL){
          xmlNode * child = cur_node->children->next;

          while(child != NULL && child != child->last){
            xmlChar * content = xmlNodeGetContent(child);
            if(strcmp((char *) child->name, TEXT) != EQUAL_STRINGS && strcmp((char *) child->name, TRKSEG) != EQUAL_STRINGS &&
               strcmp((char *) child->name, NAME) != EQUAL_STRINGS){
              gpxDataName = (char *) child->name;
              gpxDataValue = (char *) content;

              gpxData = buildGPXData(gpxData, gpxDataName, gpxDataValue);

              if(gpxData == NULL){
                parseFail = true;
                return NULL;
              }

              insertBack(track->otherData, gpxData);
            }

            xmlFree(content);
            child = child->next;
          }
        }

        insertBack(gpx->tracks, track);
      }
      else if(strcmp((char *) cur_node->name, RTE) == EQUAL_STRINGS){ 
        route = (Route *) buildRoute(route, rteName);

        if(route == NULL){
          parseFail = true;
          return NULL;
        }

        strcpy(rteName, "\0");

        if(cur_node->children != NULL){
          xmlNode * child = cur_node->children->next;

          while(child != NULL && child != child->last){
            xmlChar * content = xmlNodeGetContent(child);
            if(strcmp((char *) child->name, TEXT) != EQUAL_STRINGS && strcmp((char *) child->name, RTEPT) != EQUAL_STRINGS &&
               strcmp((char *) child->name, NAME) != EQUAL_STRINGS){
              gpxDataName = (char *) child->name;
              gpxDataValue = (char *) content;

              gpxData = buildGPXData(gpxData, gpxDataName, gpxDataValue);

              if(gpxData == NULL){
                parseFail = true;
                return NULL;
              }

              insertBack(route->otherData, gpxData);
            }

            xmlFree(content);
            child = child->next;
          }
        }

        insertBack(gpx->routes, (void *) route);
      }
    }

    if(parseFail == true){
      return NULL;
    }

    gpx = buildObjects(cur_node->children, gpx);
  }

  return gpx;
}

/* ************************************A1 FUNCTIONS**************************************** */
/** Function to create an GPX object based on the contents of an GPX file.
 *@pre File name cannot be an empty string or NULL.
       File represented by this name must exist and must be readable.
 *@post Either:
        A valid GPXdoc has been created and its address was returned
		or 
		An error occurred, and NULL was returned
 *@return the pinter to the new struct or NULL
 *@param fileName - a string containing the name of the GPX file
**/
GPXdoc * createGPXdoc(char* fileName){
    xmlDoc * doc = NULL;
    xmlNode * root_element = NULL;
    
    GPXdoc * gpx = (GPXdoc *) malloc(sizeof(GPXdoc));

    if(gpx == NULL || fileName == NULL){
      free(gpx);
      return NULL;
    }

    LIBXML_TEST_VERSION

    /*parse the file and get the DOM */
    doc = xmlReadFile(fileName, NULL, 0);

    if (doc == NULL) {
      xmlFreeDoc(doc);
      xmlCleanupParser();
      return NULL;
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);
    
    gpx = buildObjects(root_element, gpx);

    if(parseFail == true){
      deleteGPXdoc(gpx);
      xmlFreeDoc(doc);
      xmlCleanupParser();
      return NULL;
    }
    else{
      xmlFreeDoc(doc);
      xmlCleanupParser();
      return gpx;
    }
}

/** Function to create a string representation of an GPX object.
 *@pre GPX object exists, is not null, and is valid
 *@post GPX has not been modified in any way, and a string representing the GPX contents has been created
 *@return a string contaning a humanly readable representation of an GPX object
 *@param obj - a pointer to an GPX struct
**/
char * GPXdocToString(GPXdoc* doc){
  if(doc == NULL){
    return NULL;
  }

  char * gpxDocStr;
  int memLen = strlen(doc->namespace) + (sizeof(char) * DOUBLE_CHARS) + strlen(doc->creator) + 49;

  gpxDocStr = (char *) malloc(sizeof(char) * memLen);

  if(gpxDocStr == NULL){
    free(gpxDocStr);
  }

  sprintf(gpxDocStr, "\ndoc:\nnamespace: %s\nversion: %.1f\ncreator: %s\n", doc->namespace, doc->version, doc->creator);

  ListIterator iterator = createIterator(doc->waypoints);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Waypoint * waypoint = (Waypoint *) element;

    char * wptStr = waypointToString(waypoint);
    int len = strlen(wptStr);
    
    memLen += len;
    gpxDocStr = (char *) realloc(gpxDocStr, sizeof(char) * memLen);

    if(gpxDocStr == NULL){
      free(gpxDocStr);
      return NULL;
    }

    strcat(gpxDocStr, wptStr);
    free(wptStr);
  }

  iterator = createIterator(doc->routes);
  
  while((element = nextElement(&iterator)) != NULL){
    Route * route = (Route *) element;

    char * rteStr = routeToString(route);
    int len = strlen(rteStr);
    
    memLen += len;
    gpxDocStr = (char *) realloc(gpxDocStr, sizeof(char) * memLen);

    if(gpxDocStr == NULL){
      free(gpxDocStr);
      return NULL;
    }

    strcat(gpxDocStr, rteStr);
    free(rteStr);
  }

  iterator = createIterator(doc->tracks);
  
  while((element = nextElement(&iterator)) != NULL){
    Track * track = (Track *) element;

    char * trkStr = trackToString(track);
    int len = strlen(trkStr);
    
    memLen += len;
    gpxDocStr = (char *) realloc(gpxDocStr, sizeof(char) * memLen);

    if(gpxDocStr == NULL){
      free(gpxDocStr);
      return NULL;
    }

    strcat(gpxDocStr, trkStr);
    free(trkStr);
  }

  return gpxDocStr;
}

/** Function to delete doc content and free all the memory.
 *@pre GPX object exists, is not null, and has not been freed
 *@post GPX object had been freed
 *@return none
 *@param obj - a pointer to an GPX struct
**/
void deleteGPXdoc(GPXdoc* doc){
  if(doc == NULL){
    return;
  }

  free(doc->creator);
  freeList(doc->waypoints);
  freeList(doc->routes);
  freeList(doc->tracks);
  free(doc);
}

/* For the five "get..." functions below, return the count of specified entities from the file.  
They all share the same format and only differ in what they have to count.
 
 *@pre GPX object exists, is not null, and has not been freed
 *@post GPX object has not been modified in any way
 *@return the number of entities in the GPXdoc object
 *@param obj - a pointer to an GPXdoc struct
 */

//Total number of waypoints in the GPX file
int getNumWaypoints(const GPXdoc* doc){
  int waypointCount = 0;
  
  if(doc == NULL){
    return waypointCount;    
  }

  void * element;
  ListIterator iterator = createIterator(doc->waypoints);
  
	while ((element = nextElement(&iterator)) != NULL){
    waypointCount++;
  }

  return waypointCount;
}

//Total number of routes in the GPX file
int getNumRoutes(const GPXdoc* doc){
  int routeCount = 0;
  
  if(doc == NULL){
    return routeCount;
  }

  void * element;
  ListIterator iterator = createIterator(doc->routes);
  
	while ((element = nextElement(&iterator)) != NULL){
    routeCount++;
  }

  return routeCount;
}

int getNumRouteWaypoints(const Route * route){
  if(route == NULL){
    return 0;
  }
 
  int numWaypoints = 0;

  ListIterator iterator = createIterator(route->waypoints);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    numWaypoints++;
  }
  
  return numWaypoints;
}

//Total number of tracks in the GPX file
int getNumTracks(const GPXdoc* doc){
  int trackCount = 0;

  if(doc == NULL){
    return trackCount;
  }

  void * element;
  ListIterator iterator = createIterator(doc->tracks);
  
	while ((element = nextElement(&iterator)) != NULL){
    trackCount++;
  }

  return trackCount;
}

//Total number of segments in all tracks in the document
int getNumSegments(const GPXdoc* doc){
  int segmentCount = 0;

  if(doc == NULL){
    return segmentCount;
  }
  
  ListIterator iterator = createIterator(doc->tracks);
  void * element;

  while ((element = nextElement(&iterator)) != NULL){
    Track * track = (Track *) element;
    ListIterator iterator2 = createIterator(track->segments);
    void * element2;

    while ((element2 = nextElement(&iterator2)) != NULL){
      segmentCount++;
    }
  }

  return segmentCount;
}

//Total number of GPXData elements in the document
int getNumGPXData(const GPXdoc* doc){
  int gpxDataCount = 0;

  if(doc == NULL){
    return gpxDataCount;
  }

  ListIterator iterator = createIterator(doc->waypoints);
  void * element;
  
  // Count waypoint instances.
  while ((element = nextElement(&iterator)) != NULL){
    Waypoint * waypoint = (Waypoint *) element;

    if(strcmp(waypoint->name, "\0") != EQUAL_STRINGS){
      gpxDataCount++;
    }
        
    ListIterator iterator2 = createIterator(waypoint->otherData);
    void * element2;

    while ((element2 = nextElement(&iterator2)) != NULL){
      gpxDataCount++;
    }
  }

  // Count Track instances.
  iterator = createIterator(doc->tracks);
  while ((element = nextElement(&iterator)) != NULL){
    Track * track = (Track *) element;

    if(strcmp(track->name, "\0") != EQUAL_STRINGS){
      gpxDataCount++;
    }
        
    ListIterator iterator2 = createIterator(track->otherData);
    void * element2;

    while ((element2 = nextElement(&iterator2)) != NULL){
      gpxDataCount++;
    }

    ListIterator iterator3 = createIterator(track->segments);
    void * element3;

    while((element3 = nextElement(&iterator3)) != NULL){
      TrackSegment * trackSegment = (TrackSegment *) element3;
      ListIterator iterator4 = createIterator(trackSegment->waypoints);
      void * element4;

      while((element4 = nextElement(&iterator4)) != NULL){
        Waypoint * waypoint = (Waypoint *) element4;

        if(strcmp(waypoint->name, "\0") != EQUAL_STRINGS){
          gpxDataCount++;
        }

        ListIterator iterator5 = createIterator(waypoint->otherData);
        void * element5;

        while((element5 = nextElement(&iterator5)) != NULL){
          gpxDataCount++;
        }
      }
    }
  }

  // Count Route instances
  iterator = createIterator(doc->routes);

  while((element = nextElement(&iterator)) != NULL){
    Route * route = (Route *) element;

    if(strcmp(route->name, "\0") != EQUAL_STRINGS){
      gpxDataCount++;
    }

    void * element2;
    ListIterator iterator2 = createIterator(route->otherData);

    while((element2 = nextElement(&iterator2)) != NULL){
      gpxDataCount++;
    }

    iterator2 = createIterator(route->waypoints);

    while((element2 = nextElement(&iterator2)) != NULL){
      Waypoint * waypoint = (Waypoint *) element2;

      if(strcmp(waypoint->name, "\0") != EQUAL_STRINGS){
        gpxDataCount++;
      }

      ListIterator iterator3 = createIterator(waypoint->otherData);
      void * element3;

      while((element3 = nextElement(&iterator3)) != NULL){
        gpxDataCount++;
      }
    }
  }

  return gpxDataCount;
}

// Function that returns a waypoint with the given name.  If more than one exists, return the first one.  
// Return NULL if the waypoint does not exist
Waypoint* getWaypoint(const GPXdoc* doc, char* name){
  void * element;

  if(doc == NULL || name == NULL){
    return NULL;
  }
  
  // THIS LINE JUST COST ME 21%!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
  ListIterator iterator = createIterator(doc->waypoints);

  while((element = nextElement(&iterator)) != NULL){
    Waypoint * waypoint = (Waypoint *) element;
    if(strcmp(waypoint->name, name) == EQUAL_STRINGS){
      return waypoint;
    }
  }
  
  iterator = createIterator(doc->routes);
  
  while((element = nextElement(&iterator)) != NULL){
    Route * route = (Route *) element;
    ListIterator iterator2 = createIterator(route->waypoints);
    void * element2;

    while((element2 = nextElement(&iterator2)) != NULL){
      Waypoint * waypoint = (Waypoint *) element2;

      if(strcmp(waypoint->name, name) == EQUAL_STRINGS){
        return waypoint;
      }
    }
  }

  iterator = createIterator(doc->tracks);

  while ((element = nextElement(&iterator)) != NULL){
    Track * track = (Track *) element;
    ListIterator iterator2 = createIterator(track->segments);
    void * element2;

    while ((element2 = nextElement(&iterator2)) != NULL){
      TrackSegment * trackSegment = (TrackSegment *) element2;
      ListIterator iterator3 = createIterator(trackSegment->waypoints);
      void * element3;

      while((element3 = nextElement(&iterator3)) != NULL){
        Waypoint * waypoint = (Waypoint *) element3;

        if(strcmp(waypoint->name, name) == EQUAL_STRINGS){
          return waypoint;
        }
      }
    }
  }

  return NULL;
}

// Function that returns a track with the given name.  If more than one exists, return the first one. 
// Return NULL if the track does not exist 
Track* getTrack(const GPXdoc* doc, char* name){
  if(doc == NULL || name == NULL){
    return NULL;
  }

  ListIterator iterator = createIterator(doc->tracks);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Track * track = (Track *) element;

    if(strcmp(track->name, name) == EQUAL_STRINGS){
      return track;
    }
  }

  return NULL;
}

// Function that returns a route with the given name.  If more than one exists, return the first one.  
// Return NULL if the route does not exist
Route* getRoute(const GPXdoc* doc, char* name){
  if(doc == NULL || name == NULL){
    return NULL;
  }
  
  ListIterator iterator = createIterator(doc->routes);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Route * route = (Route *) element;

    if(strcmp(route->name, name) == EQUAL_STRINGS){
      return route;
    }
  }

  return NULL;
}

/* ******************************* LIST HELPER FUNCTIONS *************************** */

void deleteGpxData( void* data){
  GPXData * gpxData;
	
	if (data == NULL){
		return;
	}
	
	gpxData = (GPXData *) data;
	free(gpxData);
}

char* gpxDataToString(void * data){
  char* tmpStr;
	GPXData * gpxData;
	int len;
	
	if (data == NULL){
		return NULL;
	}
	
	gpxData = (GPXData *) data;
	len = strlen(gpxData->name) + strlen(gpxData->value) + 35;
	tmpStr = (char *) malloc(sizeof(char) * len);

  if(tmpStr == NULL){
    free(tmpStr);
    return NULL;
  }

	sprintf(tmpStr, "\tgpxData name: %s gpxData value: %s\n\n", gpxData->name, gpxData->value);
	
  //Free tmpStr after use.
	return tmpStr;  
}

int compareGpxData(const void *first, const void *second){
  GPXData * gpxData1;
	GPXData * gpxData2;
	
	if (first == NULL || second == NULL){
		return 0;
	}
	
	gpxData1 = (GPXData *) first;
	gpxData2 = (GPXData *) second;
	
	return strcmp((char *)gpxData1->name, (char *)gpxData2->name);
}

void deleteWaypoint(void * data){
  Waypoint * waypoint;
	
	if (data == NULL){
		return;
	}
	
	waypoint = (Waypoint *) data;
	
  free(waypoint->name);
	freeList(waypoint->otherData);
	free(waypoint);
}

char * waypointToString(void * data){
  char * tmpStr;
	char * otherDataStr;
  int len;

  Waypoint * waypoint;
	
	if (data == NULL){
		return NULL;
	}
	
	waypoint = (Waypoint *) data;
	len = strlen(waypoint->name) + (sizeof(char) * DOUBLE_CHARS) + (sizeof(char) * DOUBLE_CHARS) + 42;
	tmpStr = (char *) malloc (sizeof(char) * len);

  if(tmpStr == NULL){
    free(tmpStr);
    return NULL;
  }

	sprintf(tmpStr, "\tWaypoint:\n\tname: %s\n\tlat: %f lon: %f\n\n", waypoint->name, waypoint->latitude, waypoint->longitude);

  ListIterator iterator = createIterator(waypoint->otherData);
  void * element2;

  while((element2 = nextElement(&iterator)) != NULL){
    GPXData * gpxData = (GPXData *) element2;
    
    otherDataStr = gpxDataToString(gpxData);
    len += strlen(otherDataStr);
    tmpStr = (char *) realloc(tmpStr, len);

    if(tmpStr == NULL){
      free(tmpStr);
      return NULL;
    }

    strcat(tmpStr, otherDataStr);

    free(otherDataStr);
  }
	
  //Free tmpStr after use.
	return tmpStr;
}

int compareWaypoints(const void * first, const void * second){
  Waypoint * waypoint1;
	Waypoint * waypoint2;
	
	if (first == NULL || second == NULL){
		return 0;
	}
	
	waypoint1 = (Waypoint *) first;
	waypoint2 = (Waypoint *) second;
	
	return strcmp((char *) waypoint1->name, (char *) waypoint2->name);
}

void deleteRoute(void * data){
  Route * route;
	
	if (data == NULL){
		return;
	}
	
	route = (Route *) data;
	
	free(route->name);
  freeList(route->waypoints);
  freeList(route->otherData);
  free(route);
}

char * routeToString(void * data){
  char * tmpStr;
  char * waypointStr;
  char * otherDataStr;

  int len;

  if (data == NULL){
		return NULL;
	}

  Route * route = (Route *) data;
	
  len = strlen(route->name) + 25;
	tmpStr = (char *) malloc (sizeof(char) * len);

  if(tmpStr == NULL){
    free(tmpStr);
    return NULL;
  }

  sprintf(tmpStr, "\tRoute:\n\tname: %s\n\n", route->name);

  void * element;
  ListIterator iterator = createIterator(route->waypoints);

  while((element = nextElement(&iterator)) != NULL){
    Waypoint * waypoint = (Waypoint *) element;

    waypointStr = waypointToString(waypoint);
    len += strlen(waypointStr);
    tmpStr = realloc(tmpStr, len);

    if(tmpStr == NULL){
      free(tmpStr);
      return NULL;
    }

    strcat(tmpStr, waypointStr);
    free(waypointStr);
  }

  iterator = createIterator(route->otherData);

  while((element = nextElement(&iterator)) != NULL){
    GPXData * gpxData = (GPXData *) element;

    otherDataStr = gpxDataToString(gpxData);
    len += strlen(otherDataStr);
    tmpStr = realloc(tmpStr, len);

    if(tmpStr == NULL){
      free(tmpStr);
      return NULL;
    }

    strcat(tmpStr, otherDataStr);
    free(otherDataStr);
  }
	
  //Free tmpStr after use.
	return tmpStr;
}

int compareRoutes(const void * first, const void * second){
  Route * route1;
	Route * route2;
	
	if (first == NULL || second == NULL){
		return 0;
	}
	
	route1 = (Route *) first;
	route2 = (Route *) second;
	
	return strcmp((char *) route1->name, (char *) route2->name);
}


void deleteTrackSegment(void * data){
  TrackSegment * trackSegment;
	
	if (data == NULL){
		return;
	}
	
	trackSegment = (TrackSegment *) data;

	freeList(trackSegment->waypoints);
	free(trackSegment);
}

char * trackSegmentToString(void * data){
  char * tmpStr;
  char * segmentStr;

  if(data == NULL){
    return NULL;
  }

  int len = 18;

  tmpStr = (char *) malloc(sizeof(char) * len);

  if(tmpStr == NULL){
    free(tmpStr);
    return NULL;
  }

  sprintf(tmpStr, "\ttrackSegment:\n\n");

  TrackSegment * trackSegment = (TrackSegment *) data;
  ListIterator iterator = createIterator(trackSegment->waypoints);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Waypoint * waypoint = (Waypoint *) element;
    
    segmentStr = waypointToString(waypoint);
    len += strlen(segmentStr);
    tmpStr = (char *) realloc(tmpStr, len);

    if(tmpStr == NULL){
      free(tmpStr);
      return NULL;
    }

    strcat(tmpStr, segmentStr); 

    free(segmentStr);
  }
  
  //Free after use.
  return tmpStr;
}

int compareTrackSegments(const void *first, const void *second){
  TrackSegment * trackSegment1;
	TrackSegment * trackSegment2;
	
	if (first == NULL || second == NULL){
		return 0;
	}
	
	trackSegment1 = (TrackSegment *) first;
	trackSegment2 = (TrackSegment *) second;

  void * elem1;
  void * elem2;

  ListIterator iter1 = createIterator(trackSegment1->waypoints);
  ListIterator iter2 = createIterator(trackSegment2->waypoints);

  while ((elem1 = nextElement(&iter1)) != NULL && (elem2 = nextElement(&iter2)) != NULL){
    TrackSegment * trackSegment1 = (TrackSegment *) elem1;
    TrackSegment * trackSegment2 = (TrackSegment *) elem2;

		char * str1 = trackSegment1->waypoints->printData(trackSegment1);
    char * str2 = trackSegment2->waypoints->printData(trackSegment2);
		
    int result = strcmp(str1, str2);

    if(result != EQUAL_STRINGS){
      free(str1);
      free(str2);  
      return result;
    }
    
		free(str1);
    free(str2);
	}
	
	return EQUAL_STRINGS;
}

void deleteTrack(void* data){
  Track * track;
	
	if(data == NULL){
		return;
	}
	
	track = (Track *) data;
	free(track->name);
	freeList(track->segments);
  freeList(track->otherData);
	free(track);
}

char * trackToString(void * data){
  if(data == NULL){
		return NULL;
	}
  
  Track * track = (Track *) data;
  
  char * tmpStr;
  char * segmentStr;
  char * otherDataStr;

	int len = strlen(track->name) + 20;

  tmpStr = (char *) malloc(sizeof(char) * len);

  if(tmpStr == NULL){
    free(tmpStr);
    return NULL;
  }

  sprintf(tmpStr, "\tTrack:\n\tname: %s\n\n", track->name);

  ListIterator iterator = createIterator(track->segments);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    TrackSegment * trackSegment = (TrackSegment *) element;

    segmentStr = trackSegmentToString(trackSegment);
    len += strlen(segmentStr);
    tmpStr = (char *) realloc(tmpStr, len);

    if(tmpStr == NULL){
      free(tmpStr);
      return NULL;
    }

    strcat(tmpStr, segmentStr);

    free(segmentStr);
  }

  iterator = createIterator(track->otherData);

  while((element = nextElement(&iterator)) != NULL){
    GPXData * gpxData = (GPXData *) element;

    otherDataStr = gpxDataToString(gpxData);
    len += strlen(otherDataStr);
    tmpStr = (char *) realloc(tmpStr, len);

    if(tmpStr == NULL){
      free(tmpStr);
      return NULL;
    }

    strcat(tmpStr, otherDataStr);
    free(otherDataStr);
  }

  //Free tmpStr after use.
	return tmpStr;
}

int compareTracks(const void *first, const void *second){
  Track * track1;
	Track * track2;
	
	if (first == NULL || second == NULL){
		return 0;
	}
	
	track1 = (Track *) first;
	track2 = (Track *) second;
	
	return strcmp((char*)track1->name, (char*)track2->name);
}

// A2 Module 1 Helpers
xmlNode * ConvertGPXDataToXml(xmlNode * parent, GPXData * gpxData){
  xmlNode * gpxDataChild = xmlNewChild(parent, NULL, BAD_CAST gpxData->name, BAD_CAST gpxData->value);
  return gpxDataChild; // To supress warning. The node should already be connected.
}

void ConvertWaypointToXml(xmlNode * parent, Waypoint * waypoint, char * wptType){
  char latBuff[MAX_READ_CHARS];
  char lonBuff[MAX_READ_CHARS];

  xmlNodePtr newWpt = xmlNewChild(parent, NULL, BAD_CAST wptType, NULL);

  sprintf(latBuff, "%f", waypoint->latitude);
  sprintf(lonBuff, "%f", waypoint->longitude);  

  xmlNewProp(newWpt, BAD_CAST LAT, BAD_CAST latBuff);
  xmlNewProp(newWpt, BAD_CAST LON, BAD_CAST lonBuff);  

  if(strcmp(waypoint->name, "\0") != EQUAL_STRINGS){
    xmlNewChild(newWpt, NULL, BAD_CAST NAME, BAD_CAST waypoint->name);
  }

  ListIterator iterator = createIterator(waypoint->otherData);
  GPXData * gpxData;
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    gpxData = (GPXData *) element;
    ConvertGPXDataToXml(newWpt, gpxData);
  }
}

void ConvertTrackSegmentToXml(xmlNode * parent, TrackSegment * trackSegment){
  xmlNode * newTrkSeg = xmlNewChild(parent, NULL, BAD_CAST TRKSEG, NULL);

  ListIterator iterator = createIterator(trackSegment->waypoints);
  Waypoint * waypoint;
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    waypoint = (Waypoint *) element;
    ConvertWaypointToXml(newTrkSeg, waypoint, TRKPT);
  }
}

void ConvertTrackToXml(xmlNode * parent, Track * track){
  xmlNode * newTrk = xmlNewChild(parent, NULL, BAD_CAST TRK, NULL);
  
  if(strcmp(track->name, "\0") != EQUAL_STRINGS){
    xmlNewChild(newTrk, NULL, BAD_CAST NAME, BAD_CAST track->name);
  } 
  
  ListIterator iterator = createIterator(track->otherData);
  GPXData * gpxData;
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    gpxData = (GPXData *) element;
    ConvertGPXDataToXml(newTrk, gpxData);
  }

  ListIterator iterator2 = createIterator(track->segments);
  TrackSegment * trackSegment;
  void * element2;

  while((element2 = nextElement(&iterator2)) != NULL){
    trackSegment = (TrackSegment *) element2;
    ConvertTrackSegmentToXml(newTrk, trackSegment);
  }

}

void ConvertRouteToXml(xmlNode * parent, Route * route){
  xmlNode * newRte = xmlNewChild(parent, NULL, BAD_CAST RTE, NULL);
  
  if(strcmp(route->name, "\0") != EQUAL_STRINGS){
    xmlNewChild(newRte, NULL, BAD_CAST NAME, BAD_CAST route->name);
  }
  
  ListIterator iterator = createIterator(route->otherData);
  GPXData * gpxData;
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    gpxData = (GPXData *) element;
    ConvertGPXDataToXml(newRte, gpxData);
  }

  ListIterator iterator2 = createIterator(route->waypoints);
  Waypoint * waypoint;
  void * element2;

  while((element2 = nextElement(&iterator2)) != NULL){
    waypoint = (Waypoint *) element2;
    ConvertWaypointToXml(newRte, waypoint, RTEPT);
  }
}

xmlNode * ConvertGPXdocToXml(GPXdoc * gpx){
  char versionBuff[MAX_READ_CHARS];
  
  xmlNode * rootNode = xmlNewNode(NULL, BAD_CAST GPX);

  sprintf(versionBuff, "%.1f", gpx->version);

  xmlNewProp(rootNode, BAD_CAST VERSION, BAD_CAST versionBuff);
  xmlNewProp(rootNode, BAD_CAST CREATOR, BAD_CAST gpx->creator);

  // Create and set the namespace from the GPXDoc.
  xmlNs * docNs = xmlNewNs(rootNode, BAD_CAST gpx->namespace, NULL); // Use NULL as the prefix.
  xmlSetNs(rootNode, docNs); // set the namespace for the document.
  
  return rootNode;
}


xmlDoc * ConvertGPXDocToXmlDoc(GPXdoc * gpx){
  xmlDoc * doc = NULL;
  xmlNode * root;

  if(gpx == NULL){
    return NULL;
  }
  
  // Macro that checks to if the libxml version in use is compatible with the one the code is compiled against.
  LIBXML_TEST_VERSION; 

  // Create a root node and set it as the tree's root.
  doc = xmlNewDoc(BAD_CAST "1.0");
  root = ConvertGPXdocToXml(gpx);
  
  xmlDocSetRootElement(doc, root);

  ListIterator iterator = createIterator(gpx->waypoints);
  Waypoint * waypoint;
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    waypoint = (Waypoint *) element;
    ConvertWaypointToXml(root, waypoint, WPT);
  }

  ListIterator iterator2 = createIterator(gpx->routes);
  Route * route;
  void * element2;

  while((element2 = nextElement(&iterator2)) != NULL){
    route = (Route *) element2;
    ConvertRouteToXml(root, route);
  }

  ListIterator iterator3 = createIterator(gpx->tracks);
  Track * track;
  void * element3;

  while((element3 = nextElement(&iterator3)) != NULL){
    track = (Track *) element3;
    ConvertTrackToXml(root, track);
  }

  return doc;
}

bool validateXmlDoc(xmlDoc * doc, char * gpxSchemaFile){
  bool isValidXml = false;

  xmlSchema * schema = NULL;
  xmlSchemaParserCtxtPtr context;

  xmlLineNumbersDefault(1);
  context = xmlSchemaNewParserCtxt(gpxSchemaFile);

  xmlSchemaSetParserErrors(context, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);
  schema = xmlSchemaParse(context);

  xmlSchemaFreeParserCtxt(context);

  xmlSchemaValidCtxtPtr valContext;

  valContext = xmlSchemaNewValidCtxt(schema);
  xmlSchemaSetValidErrors(valContext, (xmlSchemaValidityErrorFunc) fprintf, (xmlSchemaValidityWarningFunc) fprintf, stderr);

  int retVal = -1;

  retVal = xmlSchemaValidateDoc(valContext, doc);

  if(retVal > 0){
    isValidXml = false;
  }
  else if (retVal == 0){
    isValidXml = true;
  }
  else{
    printf("Something else went wrong....\n");
  }

  if(schema != NULL){
    xmlSchemaFree(schema);
  }

  xmlFree(valContext);
  xmlSchemaCleanupTypes();
  xmlCleanupParser();
  xmlMemoryDump();

  return isValidXml; // Will return false in the else case since it doesn't change the boolean's value.
}

bool validateGPXData(GPXData * gpxData){
  if(strcmp(gpxData->name, "\0") == EQUAL_STRINGS || strcmp(gpxData->value, "\0") == EQUAL_STRINGS){
    return false;
  }

  return true;
}

bool validateWaypoint(Waypoint * waypoint){
  if(waypoint->name == NULL){
    return false;
  }

  if(waypoint->latitude == SENTINEL_LAT_LON || waypoint->latitude < MIN_LATITUDE || waypoint->latitude > MAX_LATITUDE){ // -1.0 is the sentinel value that measures invalid unset lat and lon.
    return false;
  }

  if(waypoint->longitude == SENTINEL_LAT_LON || waypoint->longitude < MIN_LONGITUDE || waypoint->longitude > MAX_LONGITUDE){
    return false;
  }

  if(waypoint->otherData == NULL){
    return false;
  }
  else{
    ListIterator iterator = createIterator(waypoint->otherData);
    void * element;

    while((element = nextElement(&iterator)) != NULL){
      GPXData * gpxData = (GPXData *) element;

      if(validateGPXData(gpxData) == false){
        return false;
      }
    }
  }

  return true;
}

bool validateRoute(Route * route){
  if(route->name == NULL){
    return false;
  }

  if(route->waypoints == NULL || route->otherData == NULL){
    return false;
  }
  else{
    ListIterator iterator = createIterator(route->otherData);
    void * element;

    while((element = nextElement(&iterator)) != NULL){
      GPXData * gpxData = (GPXData *) element;

      if(validateGPXData(gpxData) == false){
        return false;
      }
    } 

    ListIterator iterator2 = createIterator(route->waypoints);
    void * element2;

    while((element2 = nextElement(&iterator2)) != NULL){
      Waypoint * waypoint = (Waypoint *) element2;

      if(validateWaypoint(waypoint) == false){
        return false;
      }
    }
  }

  return true;
}

bool validateTrackSegment(TrackSegment * trackSegment){
  if(trackSegment->waypoints == NULL){
    return false;
  }
  else{
    ListIterator iterator = createIterator(trackSegment->waypoints);
    void * element;

    while((element = nextElement(&iterator)) != NULL){
      Waypoint * waypoint = (Waypoint *) element;

      if(validateWaypoint(waypoint) == false){
        return false;
      }
    }
  }

  return true;
}

bool validateTrack(Track * track){
  if(track->name == NULL){
    return false;
  }

  if(track->segments == NULL || track->otherData == NULL){
    return false;
  }
  else{
    ListIterator iterator = createIterator(track->segments);
    void * element;

    while((element = nextElement(&iterator)) != NULL){
      TrackSegment * trackSegment = (TrackSegment *) element;

      if(validateTrackSegment(trackSegment) == false){
        return false;
      }
    }
    
    ListIterator iterator2 = createIterator(track->otherData);
    void * element2;

    while((element2 = nextElement(&iterator2)) != NULL){
      GPXData * gpxData = (GPXData *) element2;

      if(validateGPXData(gpxData) == false){
        return false;
      }
    }
  }

  return true;
}

bool IsValidGPXdoc(GPXdoc * gpx){
  if(strcmp(gpx->namespace, "\0") == EQUAL_STRINGS){
    return false;
  }

  if(gpx->version == SENTINEL_VERSION){
    return false;
  }

  if(strcmp(gpx->creator, "\0") == EQUAL_STRINGS || gpx->creator == NULL){
    return false;
  }

  if(gpx->waypoints == NULL || gpx->routes == NULL || gpx->tracks == NULL){
    return false;
  }
  else{
    ListIterator iterator = createIterator(gpx->waypoints);
    void * element;

    while((element = nextElement(&iterator)) != NULL){
      Waypoint * waypoint = (Waypoint *) element;

      if(validateWaypoint(waypoint) == false){
        return false;
      }
    }

    ListIterator iterator2 = createIterator(gpx->routes);
    void * element2;

    while((element2 = nextElement(&iterator2)) != NULL){
      Route * route = (Route *) element2;

      if(validateRoute(route) == false){
        return false;
      }
    }

    ListIterator iterator3 = createIterator(gpx->tracks);
    void * element3;

    while((element3 = nextElement(&iterator3)) != NULL){
      Track * track = (Track *) element3;

      if(validateTrack(track) == false){
        return false;
      }
    }
  }

  return true;
}

/* ******************************* A2 functions  - MUST be implemented *************************** */

// Module 1

/** Function to create an GPX object based on the contents of an GPX file.
 * This function must validate the XML tree generated by libxml against a GPX schema file
 * before attempting to traverse the tree and create an GPXdoc struct
 *@pre File name cannot be an empty string or NULL.
       File represented by this name must exist and must be readable.
 *@post Either:
        A valid GPXdoc has been created and its address was returned
		or 
		An error occurred, and NULL was returned
 *@return the pinter to the new struct or NULL
 *@param gpxSchemaFile - the name of a schema file
 *@param fileName - a string containing the name of the GPX file
**/
GPXdoc* createValidGPXdoc(char* fileName, char* gpxSchemaFile){
  bool validXml = false;
  GPXdoc * gpx = NULL;

  xmlDoc * xDoc = xmlReadFile(fileName, NULL, 0);

  validXml = validateXmlDoc(xDoc, gpxSchemaFile);

  if(validXml == false){
    return NULL;
  }
  else{
    gpx = createGPXdoc(fileName);

    if(gpx == NULL){ // Probably redundant, but meh. I put it there to be pedantic.
      return NULL;
    }
            
    return gpx;
  }
}

/** Function to validating an existing a GPXobject object against a GPX schema file
 *@pre 
    GPXdoc object exists and is not NULL
    schema file name is not NULL/empty, and represents a valid schema file
 *@post GPXdoc has not been modified in any way
 *@return the boolean aud indicating whether the GPXdoc is valid
 *@param doc - a pointer to a GPXdoc struct
 *@param gpxSchemaFile - the name of a schema file
 **/
bool validateGPXDoc(GPXdoc * doc, char * gpxSchemaFile){
  bool validXml = false;
  bool validGPXdoc = false;

  if(doc == NULL || gpxSchemaFile == NULL || strcmp(gpxSchemaFile, "\0") == EQUAL_STRINGS){
    return false;
  }

  xmlDoc * xDoc = ConvertGPXDocToXmlDoc(doc);

  if(xDoc == NULL){
    return false;
  }

  validXml = validateXmlDoc(xDoc, gpxSchemaFile);

  if(validXml == false){
    return false;
  }
  
  validGPXdoc = IsValidGPXdoc(doc);

  if(validGPXdoc == false){
    return false;
  }
  
  return true;
}

/** Function to writing a GPXdoc into a file in GPX format.
 *@pre
    GPXdoc object exists, is valid, and and is not NULL.
    fileName is not NULL, has the correct extension
 *@post GPXdoc has not been modified in any way, and a file representing the
    GPXdoc contents in GPX format has been created
 *@return a boolean value indicating success or failure of the write
 *@param
    doc - a pointer to a GPXdoc struct
 	fileName - the name of the output file
 **/
bool writeGPXdoc(GPXdoc * doc, char * filename){
  if(doc == NULL || filename == NULL || strcmp(filename, "\0") == EQUAL_STRINGS){
    return false;
  }

  xmlDoc * xDoc = ConvertGPXDocToXmlDoc(doc);
  
  int retVal = -1;

  retVal = xmlSaveFormatFileEnc(filename, xDoc, "UTF-8", 1);
  
  if(retVal == -1){ // Then there was an error.
    return false;
  }
  else if(retVal > -1){ // Then number of bytes were written properly.
    return true;
  }

  return false; // In case somehow the retval is below -1.
}

// Haversine Formula function.
float computeDistanceBetweenWaypoints(float srcLat, float srcLon, float destLat, float destLon){
  const double earthMeanRadius = 6371e3;
  const double srcLatRadians = srcLat *  M_PI / HALF_CIRCLE_DEGREES;
  const double destLatRadians = destLat * M_PI / HALF_CIRCLE_DEGREES;
  const double distLat = (destLat - srcLat) * M_PI / HALF_CIRCLE_DEGREES;
  const double distLon = (destLon - srcLon) * M_PI / HALF_CIRCLE_DEGREES;

  const double a = sin(distLat / 2) * sin(distLat / 2) + cos(srcLatRadians) * cos(destLatRadians) * 
                  sin(distLon / 2) * sin(distLon / 2);

  const double c = 2 * atan2(sqrt(a), sqrt((1 - a)));
  float distance = (float)(earthMeanRadius * c);

  return distance;
}

// Module 2
float round10(float len){
  int div10 = (len + 5) / 10;
  div10 = div10 * 10;
  float result = (float) div10;

  return result;
}

float getRouteLen(const Route * rt){
  if(rt == NULL){
    return 0;
  }
  
  int count = 0;
  float tempLat = 0.0;
  float tempLon = 0.0;
  float routeLength = 0.0;

  void * element;
  ListIterator iterator = createIterator(rt->waypoints);
  
	while ((element = nextElement(&iterator)) != NULL){
    Waypoint * wpt = (Waypoint *) element;
    //lat: %f\tlon: %f\n", wpt->latitude, wpt->longitude);

    if(count == 0){
      tempLat = wpt->latitude;
      tempLon = wpt->longitude;
    }
    else{
      routeLength += computeDistanceBetweenWaypoints(tempLat, tempLon, wpt->latitude, wpt->longitude);
      tempLat = wpt->latitude;
      tempLon = wpt->longitude;
    }

    count++;
  }

  return routeLength;
}

float getTrackLen(const Track * tr){
  if(tr == NULL){
    return 0;
  }

  int count = 0;
  int segCount = 0;
  float trackLength = 0.0;
  float tempLat = 0.0;
  float tempLon = 0.0;
   

  void * element;
  ListIterator iterator = createIterator(tr->segments);
  
	while ((element = nextElement(&iterator)) != NULL){
    TrackSegment * trSeg = (TrackSegment *) element;

    ListIterator iterator2 = createIterator(trSeg->waypoints);
    void * element2;

    while((element2 = nextElement(&iterator2)) != NULL){
      Waypoint * wpt = (Waypoint *) element2;

      if(count == 0 && segCount == 0){ // If its the very first waypoint, then just write to the temp vals and continue on.
        tempLat = wpt->latitude;
        tempLon = wpt->longitude;
      }
      else{
        trackLength += computeDistanceBetweenWaypoints(tempLat, tempLon, wpt->latitude, wpt->longitude);
        tempLat = wpt->latitude;
        tempLon = wpt->longitude;
      }

      count++;
    }

    segCount++;
  }

  return trackLength;
}

int numRoutesWithLength(const GPXdoc * doc, float len, float delta){
  if(doc == NULL || len < 0 || delta < 0){
    return 0;
  }

  int routesWithLength = 0;

  ListIterator iterator = createIterator(doc->routes);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Route * rte = (Route *) element;
    float routeLen = getRouteLen(rte);
    float lenDifference = routeLen - len;

    if(lenDifference < 0){
      lenDifference = (lenDifference * -1); // to correct negative difference calculations.
    }

    if(lenDifference <= delta){
      routesWithLength++;
    } 
  }
  
  return routesWithLength;
}

int numTracksWithLength(const GPXdoc * doc, float len, float delta){
  if(doc == NULL || len < 0 || delta < 0){
    return 0;
  }

  int tracksWithLength = 0;
  void * element;
  ListIterator iterator = createIterator(doc->tracks);
  
	while ((element = nextElement(&iterator)) != NULL){
    Track * trk = (Track *) element;
    float trackLen = getTrackLen(trk);
    float lenDifference = trackLen - len;

    if(lenDifference < 0){
      lenDifference = (lenDifference * -1); // to correct negative difference calculations.
    }

    if(lenDifference <= delta){
      tracksWithLength++;
    } 
  }

  return tracksWithLength;
}

bool isLoopRoute(const Route * rt, float delta){
  if(rt == NULL || delta < 0){
    return false;
  }

  int numWaypoints = 0;

  float firstLat = 0.0;
  float firstLon = 0.0;
  
  float lastLat = 0.0;
  float lastLon = 0.0;

  ListIterator iterator = createIterator(rt->waypoints);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Waypoint * wpt = (Waypoint *) element;

    if(numWaypoints == 0){
      firstLat = wpt->latitude;
      firstLon = wpt->longitude;
    }
    else{
      lastLat = wpt->latitude;
      lastLon = wpt->longitude;
    }

    numWaypoints++;
  }
  
  if(numWaypoints >= MIN_LOOP_WPTS){
    float distance = computeDistanceBetweenWaypoints(firstLat, firstLon, lastLat, lastLon);
     
    if(distance <= delta){
      return true;
    }
  }

  return false;
}

bool isLoopTrack(const Track * tr, float delta){
  if(tr == NULL || delta < 0){
    return false;
  }

  int numWaypoints = 0;

  float firstLat = 0.0;
  float firstLon = 0.0;
  
  float lastLat = 0.0;
  float lastLon = 0.0;

  ListIterator iterator = createIterator(tr->segments);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    TrackSegment * trSeg = (TrackSegment *) element;

    ListIterator iterator2 = createIterator(trSeg->waypoints);
    void * element2;

    while((element2 = nextElement(&iterator2)) != NULL){
      Waypoint * wpt = NULL;
      wpt = (Waypoint *) element2;
      
      if(numWaypoints == 0){
        firstLat = wpt->latitude;
        firstLon = wpt->longitude;
      }
      else{
        lastLat = wpt->latitude;
        lastLon = wpt->longitude;
      }

      numWaypoints++;
    }
  }
  
  if(numWaypoints >= MIN_LOOP_WPTS){
    float distance = computeDistanceBetweenWaypoints(firstLat, firstLon, lastLat, lastLon);
     
    if(distance <= delta){
      return true;
    }
  }

  return false;
}

void dummyDelete(){ // Function that acts as a dummy function that doesnt actually remove anything from the list. (To preserve the GPXdoc struct).
  return;
}

List * getRoutesBetween(const GPXdoc * doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta){
  if(doc == NULL){
    return NULL;
  }

  List * routeList = initializeList(routeToString, dummyDelete, compareRoutes);
  bool routesFound = false;

  int numWaypoints = 0;

  float firstLat = 0.0;
  float firstLon = 0.0;
  
  float lastLat = 0.0;
  float lastLon = 0.0;

  ListIterator iterator = createIterator(doc->routes);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Route * route = (Route *) element;
    
    ListIterator iterator2 = createIterator(route->waypoints);
    void * element2;
  
    while((element2 = nextElement(&iterator2)) != NULL){
      Waypoint * wpt = (Waypoint *) element2;

      if(numWaypoints == 0){
        firstLat = wpt->latitude;
        firstLon = wpt->longitude;
      } 
      else{
        lastLat = wpt->latitude;
        lastLon = wpt->longitude;
      }

      numWaypoints++;
    }

    float srcDistance = computeDistanceBetweenWaypoints(sourceLat, sourceLong, firstLat, firstLon);
    float destDistance = computeDistanceBetweenWaypoints(destLat, destLong, lastLat, lastLon);

    if(srcDistance <= delta && destDistance <= delta){
      insertBack(routeList, (void *) route);
      routesFound = true;
    }
  }

  if(routesFound == false){
    return NULL;
  }
  else{
    return routeList;
  }
}

List * getTracksBetween(const GPXdoc * doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta){
  if(doc == NULL){
    return NULL;
  }

  List * trackList = initializeList(trackToString, dummyDelete, compareTracks);

  bool tracksFound = false;

  int numWaypoints = 0;

  float firstLat = 0.0;
  float firstLon = 0.0;
  
  float lastLat = 0.0;
  float lastLon = 0.0;

  ListIterator iterator = createIterator(doc->tracks);
  void * element;

  while((element = nextElement(&iterator)) != NULL){
    Track * track = (Track *) element;

    ListIterator iterator2 = createIterator(track->segments);
    void * element2;

    while((element2 = nextElement(&iterator2)) != NULL){
      TrackSegment * trkSeg = (TrackSegment *) element2;
      
      ListIterator iterator3 = createIterator(trkSeg->waypoints);
      void * element3;
    
      while((element3 = nextElement(&iterator3)) != NULL){
        Waypoint * wpt = (Waypoint *) element3;

        if(numWaypoints == 0){
          firstLat = wpt->latitude;
          firstLon = wpt->longitude;
        } 
        else{
          lastLat = wpt->latitude;
          lastLon = wpt->longitude;
        }

        numWaypoints++;
      }

      float srcDistance = computeDistanceBetweenWaypoints(sourceLat, sourceLong, firstLat, firstLon);
      float destDistance = computeDistanceBetweenWaypoints(destLat, destLong, lastLat, lastLon);

      if(srcDistance <= delta && destDistance <= delta){
        insertBack(trackList, (void *) track);
        tracksFound = true;
      }
    }
  }

  if(tracksFound == false){
    return NULL;
  }
  else{
    return trackList;
  }
}

// Module 3 
// Required Functions
char * routeListToJSON(const List * list){
  if(list == NULL || getLength((List *) list) == NO_ELEMENTS){
    char * tempStr = "[]";
    char * retStr = malloc(strlen(tempStr + 1));
    strcpy(retStr, "\0");
    strcpy(retStr, tempStr);

    return retStr;
  }

  ListIterator iterator = createIterator((List *) list);
  void * element;

  char routeListStr[JSON_LIST_STR_MAX_LENGTH];

  strcpy(routeListStr, "\0");
  strcpy(routeListStr, "[");
  int numProcessed = 0;

  while((element = nextElement(&iterator)) != NULL){
    Route * route = (Route *) element;

    char * tempRteStr = routeToJSON(route);
    //printf("route: %s strlen: %ld\n", tempRteStr, strlen(tempRteStr));
    numProcessed++;

    if(getLength((List *) list) > NO_ELEMENTS){
      if(getLength((List *) list) > numProcessed){
        strcat(routeListStr, tempRteStr);
        strcat(routeListStr, ",");
      }
      else{
        strcat(routeListStr, tempRteStr);
      }
    }

    free(tempRteStr);
  }

  strcat(routeListStr, "]");

  char* retStr = malloc(strlen(routeListStr) + 1);
  strcpy(retStr, routeListStr);
    
  return retStr;
}

char * trackListToJSON(const List * list){
  if(list == NULL || getLength((List *) list) == NO_ELEMENTS){
    char * tempStr = "[]";
    char * retStr = malloc(strlen(tempStr + 1));;
    strcpy(retStr, tempStr);

    return retStr;
  }

  ListIterator iterator = createIterator((List *) list);
  void * element;

  char trackListStr[JSON_LIST_STR_MAX_LENGTH];
  strcpy(trackListStr, "\0");
  strcpy(trackListStr, "[");

  while((element = nextElement(&iterator)) != NULL){
    Track * track = (Track *) element;

    strcat(trackListStr, trackToJSON(track) + ',');
  }

  strcat(trackListStr, "]");

  char* retStr = malloc(strlen(trackListStr) + 1);

  strcpy(retStr, trackListStr);
    
  return retStr;
}

char * trackToJSON(const Track * tr){
  if(tr == NULL){
    char * tempStr = "{}";
    char * retStr = malloc(strlen(tempStr + 1));;
    strcpy(retStr, tempStr);

    return retStr;
  }  

  char trackStr[JSON_STR_LEN];
  char nameStr[JSON_NAME_LEN];
  char loopStr[MAX_READ_CHARS];

  strcpy(trackStr, "\0");
  strcpy(nameStr, "\0");
  strcpy(loopStr, "\0");

  if(strcmp(tr->name, "\0") == EQUAL_STRINGS){
    strcpy(nameStr, "None");
  }  
  else{
    strcpy(nameStr, tr->name);
  }

  if(isLoopTrack(tr, DEFAULT_DELTA) == true){
    strcpy(loopStr, "true");
  }
  else{
    strcpy(loopStr, "false");
  }

  sprintf(trackStr, "{\"name\":\"%s\",\"len\":%.1f,\"loop\":%s}", nameStr, round10(getTrackLen(tr)), loopStr);
  
  char* retStr = malloc(strlen(trackStr) + 1);
  strcpy(retStr, trackStr);
    
  return retStr;
}

char * routeToJSON(const Route * rt){
  if(rt == NULL){
    char * tempStr = "{}";
    char * retStr = malloc(strlen(tempStr + 1));;
    strcpy(retStr, tempStr);

    return retStr;
  }

  char routeStr[JSON_STR_LEN];
  char nameStr[JSON_NAME_LEN];
  char loopStr[MAX_READ_CHARS];
  strcpy(routeStr, "\0");
  strcpy(nameStr, "\0");
  strcpy(loopStr, "\0");

  if(strcmp(rt->name, "\0") == EQUAL_STRINGS){
    strcpy(nameStr, "None");
  }  
  else{
    strcpy(nameStr, rt->name);
  }

  if(isLoopRoute(rt, DEFAULT_DELTA) == true){
    strcpy(loopStr, "true");
  }
  else{
    strcpy(loopStr, "false");
  }

  sprintf(routeStr, "{\"name\":\"%s\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s}", nameStr, getNumRouteWaypoints(rt), round10(getRouteLen(rt)), loopStr);
  
  char* retStr = malloc(strlen(routeStr) + 1);
  strcpy(retStr, routeStr);
    
  return retStr;
}

char * GPXtoJSON(const GPXdoc * gpx){
  if(gpx == NULL){
    char * tempStr = "{}";
    char * retStr = malloc(strlen(tempStr + 1));;
    strcpy(retStr, tempStr);

    return retStr;
  }

  char gpxStr[JSON_GPX_STR_MAX_LENGTH];

  sprintf(gpxStr, "{\"version\":%.1f,\"creator\":\"%s\",\"numWaypoints\":%d,\"numRoutes\":%d,\"numTracks\":%d}", gpx->version, gpx->creator, getNumWaypoints(gpx), getNumRoutes(gpx), getNumTracks(gpx));
  
  char* retStr = malloc(strlen(gpxStr) + 1);
  strcpy(retStr, gpxStr);
    
  return retStr;
}

// Bonus Functions
void addWaypoint(Route * rt, Waypoint * pt){
  if(rt == NULL || pt == NULL){
    return;
  }

  insertBack(rt->waypoints, (void *) pt);
}  

void addRoute(GPXdoc * doc, Route * rt){
  if(doc == NULL || rt == NULL){
    return;
  }

  insertBack(doc->routes, (void *) rt);
}

char * TrimParentheses(char * str){
  for(int i = 0; i < strlen(str); i++){
    str[i] = str[i + 1];
  }

  str[strlen(str) - 2] = '\0';

  return str;
}

GPXdoc * JSONtoGPX(const char * gpxString){
  if(gpxString == NULL || strcmp(gpxString, "\0") == EQUAL_STRINGS){
    return NULL;
  }

  char * commaParse[2];
  char delim[2] = ",";
  char * token;

  int cParseCount = 0;

  token = strtok((char *) gpxString, delim);
   
  while(token != NULL) {
    commaParse[cParseCount] = token;
    cParseCount++;
    token = strtok(NULL, delim);
  }

  char * colonParse[4];
  int clnParseCount = 0;
  delim[0] = ':';

  for(int i = 0; i < 2; i++){
    token = strtok((char *) commaParse[i], delim);
   
    while(token != NULL) {
      colonParse[clnParseCount] = token;
      clnParseCount++;
      token = strtok(NULL, delim);
    }
  }

  char version[256];
  char creator[256];

  strcpy(version, colonParse[1]);
  strcpy(creator, colonParse[3]);

  strcpy(creator, TrimParentheses(creator));

  GPXdoc * doc = NULL;

  doc = buildGPXdoc(doc, DEFAULT_NAMESPACE, version, creator);

  return doc;
}

Waypoint * JSONtoWaypoint(const char * gpxString){
  if(gpxString == NULL || strcmp(gpxString, "\0") == EQUAL_STRINGS){
    return NULL;
  }

  char * commaParse[2];
  char delim[2] = ",";
  char * token;

  int cParseCount = 0;

  token = strtok((char *) gpxString, delim);
   
  while(token != NULL) {
    commaParse[cParseCount] = token;
    cParseCount++;
    token = strtok(NULL, delim);
  }

  char * colonParse[4];
  int clnParseCount = 0;
  delim[0] = ':';

  for(int i = 0; i < 2; i++){
    token = strtok((char *) commaParse[i], delim);
   
    while(token != NULL) {
      colonParse[clnParseCount] = token;
      clnParseCount++;
      token = strtok(NULL, delim);
    }
  }

  char latStr[256];
  char lonStr[256];

  strcpy(latStr, colonParse[1]);
  strcpy(lonStr, colonParse[3]);

  lonStr[strlen(lonStr) - 1] = '\0';

  Waypoint * wpt = NULL;

  wpt = buildWaypoint(wpt, "\0", lonStr, latStr);

  return wpt;
}

Route * JSONtoRoute(const char * gpxString){
  if(gpxString == NULL || strcmp(gpxString, "\0") == EQUAL_STRINGS){
    return NULL;
  }

  char delim[2] = ":";
  char * token;

  char * colonParse[2];
  int clnParseCount = 0;

  token = strtok((char *) gpxString, delim);
  
  while(token != NULL) {
    colonParse[clnParseCount] = token;
    clnParseCount++;
    token = strtok(NULL, delim);
  }

  char name[256];

  strcpy(name, TrimParentheses(colonParse[1]));

  Route * rte = NULL;

  rte = buildRoute(rte, name);

  return rte;
}

// Additional Functions
char * waypointToJSON(const Waypoint * wpt){
  if(wpt == NULL){
    char * tempStr = "{}";
    char * retStr = malloc(strlen(tempStr + 1));
    strcpy(retStr, tempStr);

    return retStr;
  }

  char wptStr[JSON_WPT_STR_LEN] = "\0";
  char nameStr[JSON_NAME_LEN] = "\0";

  //strcpy(wptStr, "\0");
  //strcpy(nameStr, "\0");
  //strcpy(loopStr, "\0");

  if(strcmp(wpt->name, "\0") == EQUAL_STRINGS){
    strcpy(nameStr, "None");
  }  
  else{
    strcpy(nameStr, wpt->name);
  }

  sprintf(wptStr, "{\"name\":\"%s\",\"latitude\":%f,\"longitude\":%f}", nameStr, wpt->latitude, wpt->longitude);
  
  char* retStr = malloc(strlen(wptStr) + 1);
  strcpy(retStr, wptStr);
    
  return retStr;
}

char * getJSONRoutePointList(const List * list){
  if(list == NULL || getLength((List *) list) == NO_ELEMENTS){
      char * tempStr = "[]";
      char * retStr = malloc(strlen(tempStr + 1));
      strcpy(retStr, "\0");
      strcpy(retStr, tempStr);

      return retStr;
    }

    ListIterator iterator = createIterator((List *) list);
    void * element;

    char routePointListStr[JSON_LIST_STR_MAX_LENGTH];

    strcpy(routePointListStr, "\0");
    strcpy(routePointListStr, "[");
    int numProcessed = 0;

    while((element = nextElement(&iterator)) != NULL){
      Waypoint * waypoint = (Waypoint *) element;

      char * tempPntListStr = waypointToJSON(waypoint);
      numProcessed++;
      if(getLength((List *) list) > NO_ELEMENTS){
        if(getLength((List *) list) > numProcessed){
          strcat(routePointListStr, tempPntListStr);
          strcat(routePointListStr, ",");  
        }
        else{
          strcat(routePointListStr, tempPntListStr);
        }
      }

      free(tempPntListStr); 
    }

    strcat(routePointListStr, "]");

    char* retStr = malloc(strlen(routePointListStr) + 1);
    strcpy(retStr, routePointListStr);
      
    return retStr;
  }

bool createGPXFileFromJSON(char * filename, char * creator, char * version, char * gpxSchemaFile){
  if(strcmp(filename, "\0") == EQUAL_STRINGS || strcmp(creator, "\0") == EQUAL_STRINGS || strcmp(version, "\0") == EQUAL_STRINGS || strcmp(gpxSchemaFile, "\0") == EQUAL_STRINGS){
    return false;
  }

  GPXdoc * newGpx = NULL;
  newGpx = buildGPXdoc(newGpx, DEFAULT_NAMESPACE, version, creator);

  if(validateGPXDoc(newGpx, gpxSchemaFile) == true){
    if(writeGPXdoc(newGpx, filename) == true){
      deleteGPXdoc(newGpx);
      return true; 
    }
    else{
      deleteGPXdoc(newGpx);
      return false;
    }
  }

  deleteGPXdoc(newGpx);
  
  return false;
}

char * getGPXSummary(char * filename){
  GPXdoc * currentGpx = createGPXdoc(filename);

  if(currentGpx == NULL){
    char err[10] = "error!\0";

    char * errStr = malloc(strlen(err) + 1);
    deleteGPXdoc(currentGpx);
    return errStr;
  }

  char gpxStr[JSON_STR_LEN];
  strcpy(gpxStr, "\0");
  strcat(gpxStr, GPXtoJSON(currentGpx)); 

  char * retStr = malloc(strlen(gpxStr) + 1);
  strcpy(retStr, "\0");
  strcpy(retStr, gpxStr);
  deleteGPXdoc(currentGpx);
  return retStr;
}

bool isValidGPXFile(char * filename, char * gpxSchemaFile){
  GPXdoc * fileGPX = createGPXdoc(filename);
  bool isValid = validateGPXDoc(fileGPX, gpxSchemaFile);

  return isValid;
}



char * getJSONGPXRoutePointList(char * validGPXFile){
  GPXdoc * fileGPX = createGPXdoc(validGPXFile);

  if(getLength(fileGPX->routes) == NO_ELEMENTS){

    char * emptyStr = "{\"routes\": []}";
    char * retStr = malloc(strlen(emptyStr + 1));
    strcpy(retStr, "\0");
    strcpy(retStr, emptyStr);

    return retStr;
  }

  char fileData[FILE_JSON_STR_LEN];
  strcpy(fileData, "\0");
  strcat(fileData, "{\"routes\":");
  char * tempRteStr = routeListToJSON(fileGPX->routes);
  if(strcmp(tempRteStr, "[]") == EQUAL_STRINGS){
    strcat(fileData, " []}");
    free(tempRteStr);
  }
  else{
    strcat(fileData, tempRteStr);
    free(tempRteStr);
    strcat(fileData, ",\"points\":{");

    ListIterator iterator = createIterator(fileGPX->routes);
    void * element;

    int numProcessed = 0;
    while((element = nextElement(&iterator)) != NULL){
      Route * rte = (Route *) element;
      char tag[500];
      strcpy(tag, "\0");
      sprintf(tag, "\"wpts%d\":", (numProcessed + 1));
      strcat(fileData, tag);
      char * rtePoints = getJSONRoutePointList(rte->waypoints);
      numProcessed++;
      strcat(fileData, rtePoints);
      if(numProcessed < getLength(rte->waypoints)){
        strcat(fileData, ",");
      }

      free(rtePoints);
    }

    fileData[(strlen(fileData) - 1)] = '}';
  }

  if(fileData[strlen(fileData) - 1] == '}' && fileData[strlen(fileData) - 2] == '['){
    fileData[strlen(fileData) - 1] = ']';
    strcat(fileData, "}");
  }

  strcat(fileData, "}");

  char * retStr = malloc(strlen(fileData) + 1);
  strcpy(retStr, "\0");
  strcpy(retStr, fileData);
  deleteGPXdoc(fileGPX);

  return retStr;
}
