/*                                                                                                                                                                                                                                                                              
 * videoToQuad
 *
 * Copyright (c) 2014-2015 FOXEL SA - http://foxel.ch
 * Please read <http://foxel.ch/license> for more information.
 *
 * Author(s):
 *
 *      Luc Deschenaux <l.deschenaux@foxel.ch>
 *
 * This file is part of the FOXEL project <http://foxel.ch>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Additional Terms:
 *
 *      You are required to preserve legal notices and author attributions in
 *      that material or in the Appropriate Legal Notices displayed by works
 *      containing it.
 *
 *      You are required to attribute the work as explained in the "Usage and
 *      Attribution" section of <http://foxel.ch/license>.
 */
 

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <getopt.h>
#include <string>
#include <algorithm>
#include <cstdlib>

#define cimg_use_ffmpeg
#include "perspectiveTransform.hpp"

typedef struct video_pb {
  const char *filename;
  unsigned int fps;
  const char *codec;
  unsigned int bitrate;
} video_pb;

// corner index
const int topL=0,topR=1,botL=2,botR=3;

struct MatchPathSeparator {
  bool operator()( char ch ) const {
    return ch == '/';
  }
};

std::string basename( std::string const& pathname ) {
      return std::string(std::find_if( pathname.rbegin(), pathname.rend(), MatchPathSeparator() ).base(), pathname.end());
} // basename

void usage(const char *appName) {

  appName=basename(std::string(appName)).c_str();

  fprintf(stderr,"\n\
NAME\n\
\t%s - Video to quadrilateral\n\n\
SYNOPSIS\n\
\t%s [OPTION ...]\n\n\
DESCRIPTION\n\
\tProject the input video into the quadrilateral shape defined by the extreme\n\tcoordinates of transparent pixels in the mask.\n\n\
OPTIONS\n\
\t-i, --video-in <filename>\n\
\t\tinput video file\n\n\
\t-m, --mask <filename>\n\
\t\toutput frame mask\n\n\
\t-o, --video-out <filename>\n\
\t\toutput video name\n\n\
  ",appName,appName,appName);
  exit(1);

} // usage

// get transparent quadrilateral mask corners
int getMaskCorners(CImg<unsigned char> &output_mask,ptPoint *corners) {

  int width=output_mask.width();
  int height=output_mask.height();

  ptPoint *topL=corners;
  ptPoint *topR=corners+1;
  ptPoint *botR=corners+2;
  ptPoint *botL=corners+3;

  float topLsd=-1;
  float topRsd=-1;
  float botLsd=-1;
  float botRsd=-1;


  for (float y=0; y<height; ++y) {

    float yy=y*y;
    float y2=height-y;
    float yy2=y2*y2;

    for (float x=0; x<width; ++x) {

      float xx=x*x;
      float x2=width-x; 
      float xx2=x2*x2;

      // skip non-transparent pixels
      if (output_mask(x,y,0,3)) {
        continue;
      }

      /* Assume that pixels with the least square distance
       * from image corners are the quad corners.
       */

      // top left corner
      float sd=xx+yy;
      if (topLsd<0 || sd<topLsd) {
        topLsd=sd;
        topL->x=x;
        topL->y=y;
      }

      // bottom left corner
      sd=xx+yy2;
      if (botLsd<0 || sd<botLsd) {
        botLsd=sd;
        botL->x=x;
        botL->y=y;
      }
    
      // top right corner
      sd=xx2+yy;
      if (topRsd<0 || sd<topRsd) {
        topRsd=sd;
        topR->x=x;
        topR->y=y;
      }

      // bottom right corner
      sd=xx2+yy2;
      if (botRsd<0 || sd<botRsd) {
        botRsd=sd;
        botR->x=x;
        botR->y=y;
      }

    }
  }

  return topLsd<0 || topRsd<0 || botLsd<0 || botRsd<0;


} // getMaskCorners

std::string leadingZeros(int n, int value) {
  std::string s=std::to_string(value);
  s.insert(0,n-s.length(),'0');
  return s;
}

long frame_begin=-1;
long frame_end=-1;
long frame_step=1;

int videoToQuad(std::string &video_in, std::string &mask, video_pb *video_out,std::string &output_folder) {
    
  CImgList<unsigned char> input_frames;
  CImg<unsigned char> output_mask(mask.c_str());
  ptPoint mask_corners[4];

  if (getMaskCorners(output_mask,mask_corners)) {
    fprintf(stderr,"error: Could not find mask corners\n");
    return -1;
  }

  fprintf(stderr,"info: loading %s\n",video_in.c_str());

  if (frame_begin==-1) frame_begin=0;
  if (frame_end==-1) frame_end=0;
  input_frames.load_ffmpeg(video_in.c_str(),frame_begin,frame_end,frame_step);

  fprintf(stderr,"info: resolution %dx%d, %d frames\n\n",input_frames.begin()->width(),input_frames.begin()->height(),input_frames.size());

  double *H=getPerspectiveTransform(mask_corners);

  CImgList<unsigned char> output_frames;
  int count=0;

  for (CImgList<unsigned char>::iterator frame=input_frames.begin(); frame<input_frames.end(); ++frame) {
    CImg<unsigned char> output_frame(output_mask);
    fprintf(stderr,"\rinfo: processing frame %d ",count*frame_step+frame_begin);
    inversePerspectiveTransformImage(frame,&output_frame,NULL,H,true);
    if (video_out->filename) {
      output_frames.push_back(output_frame);
    } else {
      std::string output_filename=output_folder+"/"+leadingZeros(6,count*frame_step+frame_begin)+".png";
      output_frame.save(output_filename.c_str());
    }
    ++count;
  }

  input_frames=NULL;

  if (video_out->filename) {
    fprintf(stderr,"info: saving result to %s\n",video_out->filename);
    output_frames.save_ffmpeg_external(video_out->filename,video_out->codec,video_out->fps,video_out->bitrate);
  }

  return 0;

} // videoToQuad

int main(int argc, char **argv) {

   int c;
   int digit_optind = 0;

   std::string video_in;
   std::string mask;
   video_pb video_out;
   std::string output_folder;

   video_out.filename=0;
   video_out.codec=0;
   video_out.bitrate=2048;
   video_out.fps=25;
   
   while (1) {
       int this_option_optind = optind ? optind : 1;
       int option_index = 0;
       static struct option long_options[] = {
           {"video-in",       required_argument, 0,  'i' },
           {"mask",           required_argument, 0,  'm' },
           {"target-folder",  required_argument, 0,  't' },
           {"video-out",      required_argument, 0,  'o' },
           {"help",           no_argument,       0,  'h' },
           {"codec",          required_argument, 0,  'c' },
           {"bitrate",        required_argument, 0,  'b' },
           {"fps",            required_argument, 0,  'f' },
           {"frame-begin",    required_argument, 0,  'B' },
           {"frame-end",      required_argument, 0,  'E' },
           {"frame-step",     required_argument, 0,  'S' },
           {0,                0,                 0,   0  }
       };

       c = getopt_long(argc, argv, "i:m:o:hc:b:f:t:B:E:S:",
                long_options, &option_index);
       if (c == -1)
           break;

       switch (c) {
       case 'i':
           video_in=std::string(optarg);
           break;

       case 'm':
           mask=std::string(optarg);
           break;

       case 'o':
           video_out.filename=std::string(optarg).c_str();
           break;

       case 'c':
           video_out.codec=std::string(optarg).c_str();
           break;

       case 'b':
           video_out.bitrate=atoi(optarg);
           break;

       case 'f':
           video_out.fps=atoi(optarg);
           break;

       case 't':
           output_folder=std::string(optarg);
           break;

       case 'h':
           usage(argv[0]);
           break;

       case 'B':
           frame_begin=atoi(optarg);
           break;

       case 'E':
           frame_end=atoi(optarg);
           break;

       case 'S':
           frame_step=atoi(optarg);
           break;

       case '?':
           break;

       default:
           printf("?? getopt returned character code 0%o ??\n", c);
       }
   }

   if (optind < argc) {
       fprintf(stderr,"\nInvalid argument: ");
       while (optind < argc)
           fprintf(stderr,"%s ", argv[optind++]);
       fprintf(stderr,"\n");
       usage(argv[0]);
   }

   if ((!video_out.filename && !output_folder.length()) || (video_out.filename && output_folder.length())) {
     fprintf(stderr,"\nerror: You must specify --video-out OR --target-folder\n\n");
     exit(1);
   }

   return videoToQuad(video_in,mask,&video_out,output_folder);

} // main


