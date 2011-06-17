// ----------------------------------------------------------------------------
//
// Bgeo-Write.cc
//
// Copyright (c) 2011 Exotic Matter AB.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of Exotic Matter AB nor its contributors may be used to
//   endorse or promote products derived from this software without specific
//   prior written permission.
//
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT
//    LIMITED TO,  THE IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS
//    FOR  A  PARTICULAR  PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL THE
//    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//    BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS  OR  SERVICES;
//    LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER
//    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT
//    LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN
//    ANY  WAY OUT OF THE USE OF  THIS SOFTWARE,  EVEN IF ADVISED OF  THE
//    POSSIBILITY OF SUCH DAMAGE.
//
// ----------------------------------------------------------------------------

// Naiad Base API
#include <NbFilename.h>
#include <NbBlock.h>

// Naiad Graph API
#include <NgBodyOp.h>
#include <NgProjectPath.h>

// Naiad Interface
#include <Ni.h>

#include <stdint.h>
#include "../common/Bgeo.h"
using namespace std;

// Our "Bgeo write" body op class:

class Bgeo_Write : public Ng::BodyOp
{
public:
	Bgeo_Write(const Nb::String& name)
        : Ng::BodyOp(name) {}

    virtual Nb::String
    typeName() const
    { return "Bgeo-Write"; }

    virtual void
    stepAdmittedBody(Nb::Body*             body,
                     Ng::NelContext&       nelContext,
                     const Nb::TimeBundle& tb)
    {
    	cout << "Welcome" << endl;
    	const Nb::String bodyNameList = param1s("Body Names")->eval(tb);

        // skip bodies not listed in "Body Names" ...
        if(!body->name().listed_in(bodyNameList))
            return;

        // expand output filename for each frame
        const int        padding = param1i("Frame Padding")->eval(tb);
        const Nb::String sequenceName = param1s("Output Filename")->eval(tb);
        Nb::String expandedFilename = Nb::sequenceToFilename(
            Ng::projectPath(),
            sequenceName,
            tb.frame,
            tb.timestep,
            padding
            );

        std::ofstream os(expandedFilename.str().c_str());
        if(!os.good())
            NB_THROW("Cannot create output file: '" << expandedFilename << "'");

    	if(body->matches("Mesh")) {
    		//WRITE MESH BGEO
    		const Nb::PointShape& point =  body->constPointShape();
    		const uint32_t nPoints= point.channel(0)->size();
    		const uint32_t nPointAtr = point.channelCount() - 1; // position doesn't count

    		cout << "Point channel names: " ;
    		for(int i=0; i<nPointAtr; ++i) {
				cout << point.channel(i)->name() << " ";
    		}
    		cout << endl;
    		const Nb::TriangleShape& triangle = body->constTriangleShape();
    		const uint32_t nPrims= triangle.channel(0)->size();
    		uint32_t nVtxAtr = 0;
    		cout << "Triangle channel names: " ;
    		int totalTriangleAtr = triangle.channelCount() - 1; //index doesn't count
    		for(int i=0; i<triangle.channelCount(); ++i) {
				cout << triangle.channel(i)->name() << " " << triangle.channel(i)->typeName() << endl << triangle.channel(i)->defaultValue()[0]<<  triangle.channel(i)->defaultValue()[1]<< triangle.channel(i)->defaultValue()[2]<< endl ;
				if (triangle.channel(i)->name().find("$0") != string::npos)
					++nVtxAtr;
				if ((triangle.channel(i)->name().find("$1") != string::npos) || (triangle.channel(i)->name().find("$2") != string::npos))
					--totalTriangleAtr;
    		}
    		cout << endl;
    		uint32_t vNr = 5;
    		uint32_t nPrimAtr = totalTriangleAtr - nVtxAtr;
    		uint32_t nAtr = 0; // no idea what detailAtr is used for
    		uint32_t nPointGrps = 0; // no idea what nPointGrps is used for
    		uint32_t nPrimGrps = 0; // no idea what nPrimGrps is used for
    		uint32_t paraArr[9]={vNr,nPoints,nPrims,nPointGrps,nPrimGrps,nPointAtr,nVtxAtr,nPrimAtr,nAtr};
    		cout << "nPoints:" << nPoints << endl;
    		cout << "nPrims: " << nPrims << endl;
    		cout << "nPointAtr: " << nPointAtr << endl;
    		cout << "nVtxAtr: " << nVtxAtr << endl;
    		cout << "nPrimAtr: " << nPrimAtr << endl;

    		char headrStr[] = "BgeoV";
    		os.write(headrStr,5);
    		os.write( (char*) paraArr, sizeof(uint32_t)*9);


			//triangle.channel(0)->defaultValue()
    		//Buffer3i& b = triangle.constBuffer3i(0);

    		//b.size()
    	//	b(i)


    	} else if(body->matches("Particle")) {


    	// WRITE PARTICLE BGEO
    	} else
    		NB_WARNING("Skipping body " << body->name() << " because it does not match Mesh or Particle signature.")



        // figure out what (if any) scaling needs to be applied...
        const Nb::String uc=param1e("Unit Conversion")->eval(tb);
        float scale=1;
        if(uc=="Meter to Decimeter")
            scale=10.f;
        else if(uc=="Meter to Centimeter")
            scale=100.f;
        else if(uc=="Meter to Millimeter")
            scale=1000.f;

        // now write the particle channels ...

        const Nb::ParticleShape& particle = body->constParticleShape();

        // export each particle channel in turn:

        for(int i=0; i<particle.channelCount(); ++i) {

            // write channel name
            os << particle.channel(i)->name() << ":" << std::endl;

            // write channel data
            switch(particle.channel(i)->type()) {

                // scalar floating particle channels:
                case Nb::ValueBase::FloatType:
                {
                    const Nb::BlockArray1f& blocks=particle.constBlocks1f(i);
                    const int bcount=blocks.block_count();
                    for(int b=0; b<bcount; ++b) {
                        const Nb::Block1f& cb = blocks(b);
                        for(int64_t p(0); p<cb.size(); ++p)
                            os << cb(p) << std::endl;
                    }
                }
                break;

                // scalar integer particle channels:
                case Nb::ValueBase::IntType:
                {
                    const Nb::BlockArray1i& blocks=particle.constBlocks1i(i);
                    const int bcount=blocks.block_count();
                    for(int b=0; b<bcount; ++b) {
                        const Nb::Block1i& cb = blocks(b);
                        for(int64_t p(0); p<cb.size(); ++p)
                            os << cb(p) << std::endl;
                    }
                }
                break;

                // scalar int64 particle channels:
                case Nb::ValueBase::Int64Type:
                {
                    const Nb::BlockArray1i64& blocks=
                        particle.constBlocks1i64(i);
                    const int bcount=blocks.block_count();
                    for(int b=0; b<bcount; ++b) {
                        const Nb::Block1i64& cb = blocks(b);
                        for(int64_t p(0); p<cb.size(); ++p)
                            os << cb(p) << std::endl;
                    }
                }
                break;

                // vec3f particle channels:
                case Nb::ValueBase::Vec3fType:
                {
                    const Nb::BlockArray3f& blocks=particle.constBlocks3f(i);
                    const int bcount=blocks.block_count();
                    // apply scale on Particle.position and Particle.velocity
                    // channels - in a real op, the user should probably
                    // be allowed so specify to which channels the scale
                    // should be applied...
                    if(particle.channel(i)->name()=="position" ||
                       particle.channel(i)->name()=="velocity") {
                        for(int b=0; b<bcount; ++b) {
                            const Nb::Block3f& cb = blocks(b);
                            for(int64_t p(0); p<cb.size(); ++p)
                                os << cb(p)*scale << std::endl;
                        }
                    } else {
                        for(int b=0; b<bcount; ++b) {
                            const Nb::Block3f& cb = blocks(b);
                            for(int64_t p(0); p<cb.size(); ++p)
                                os << cb(p) << std::endl;
                        }
                    }
                }
                break;

                default:
                    NB_THROW("Channel type " <<
                             particle.channel(i)->typeName() << " not supported");
            }
        }
    }
};

// ----------------------------------------------------------------------------

// Register and upload this user op to the dynamics server

extern "C" Ng::Op*
NiUserOpAlloc(const Nb::String& name)
{
    return new Bgeo_Write(name);
}

// ----------------------------------------------------------------------------
