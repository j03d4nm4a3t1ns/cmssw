#ifndef CSCSegment_CSCSegmentBuilderPluginFactory_h
#define CSCSegment_CSCSegmentBuilderPluginFactory_h

/** \class CSCSegmentBuilderPluginFactory
 *  Plugin factory for concrete CSCSegmentBuilder algorithms
 *
 * $Date: 2007/03/08 14:46:20 $
 * $Revision: 1.4 $
 * \author M. Sani
 * 
 */

#include <FWCore/PluginManager/interface/PluginFactory.h>
#include <RecoLocalMuon/CSCSegment/src/CSCSegmentAlgorithm.h>

class edm::ParameterSet;

typedef edmplugin::PluginFactory<CSCSegmentAlgorithm *(const edm::ParameterSet&)> CSCSegmentBuilderPluginFactory;

#endif
