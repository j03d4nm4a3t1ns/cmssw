#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "DataFormats/L1THGCal/interface/HGCFETriggerDigi.h"
#include "DataFormats/L1THGCal/interface/HGCFETriggerDigiFwd.h"
#include "DataFormats/HGCDigi/interface/HGCDigiCollections.h"
#include "DataFormats/ForwardDetId/interface/HGCTriggerDetId.h"

#include "L1Trigger/L1THGCal/interface/HGCalTriggerGeometryBase.h"
#include "L1Trigger/L1THGCal/interface/HGCalTriggerFECodecBase.h"
#include "L1Trigger/L1THGCal/interface/HGCalTriggerBackendProcessor.h"

#include <sstream>
#include <memory>

class HGCalTriggerDigiFEReproducer : public edm::EDProducer 
{  
    public:    
        HGCalTriggerDigiFEReproducer(const edm::ParameterSet&);
        ~HGCalTriggerDigiFEReproducer() { }

        virtual void beginRun(const edm::Run&, const edm::EventSetup&);
        virtual void produce(edm::Event&, const edm::EventSetup&);

    private:
        // inputs
        edm::EDGetToken inputdigi_;
        // algorithm containers
        std::unique_ptr<HGCalTriggerGeometryBase> triggerGeometry_;
        std::unique_ptr<HGCalTriggerFECodecBase> codec_;
        std::unique_ptr<HGCalTriggerBackendProcessor> backEndProcessor_;
};

DEFINE_FWK_MODULE(HGCalTriggerDigiFEReproducer);


/*****************************************************************/
HGCalTriggerDigiFEReproducer::HGCalTriggerDigiFEReproducer(const edm::ParameterSet& conf):
    inputdigi_(consumes<l1t::HGCFETriggerDigiCollection>(conf.getParameter<edm::InputTag>("feDigis")))
/*****************************************************************/
{
    //setup geometry configuration
    const edm::ParameterSet& geometryConfig = conf.getParameterSet("TriggerGeometry");
    const std::string& trigGeomName = geometryConfig.getParameter<std::string>("TriggerGeometryName");
    HGCalTriggerGeometryBase* geometry = HGCalTriggerGeometryFactory::get()->create(trigGeomName,geometryConfig);
    triggerGeometry_.reset(geometry);

    //setup FE codec
    const edm::ParameterSet& feCodecConfig =  conf.getParameterSet("FECodec");
    const std::string& feCodecName = feCodecConfig.getParameter<std::string>("CodecName");
    HGCalTriggerFECodecBase* codec = HGCalTriggerFECodecFactory::get()->create(feCodecName,feCodecConfig,triggerGeometry_.get());
    codec_.reset(codec);
    codec_->unSetDataPayload();

    produces<l1t::HGCFETriggerDigiCollection>();
    //setup BE processor
    backEndProcessor_ = std::make_unique<HGCalTriggerBackendProcessor>(conf.getParameterSet("BEConfiguration"), triggerGeometry_.get());
    // register backend processor products
    backEndProcessor_->setProduces(*this);
}

/*****************************************************************/
void HGCalTriggerDigiFEReproducer::beginRun(const edm::Run& /*run*/, const edm::EventSetup& es) 
/*****************************************************************/
{
    triggerGeometry_->reset();
    HGCalTriggerGeometryBase::es_info info;
    const std::string& ee_sd_name = triggerGeometry_->eeSDName();
    const std::string& fh_sd_name = triggerGeometry_->fhSDName();
    const std::string& bh_sd_name = triggerGeometry_->bhSDName();
    es.get<IdealGeometryRecord>().get(ee_sd_name,info.geom_ee);
    es.get<IdealGeometryRecord>().get(fh_sd_name,info.geom_fh);
    es.get<IdealGeometryRecord>().get(bh_sd_name,info.geom_bh);
    es.get<IdealGeometryRecord>().get(ee_sd_name,info.topo_ee);
    es.get<IdealGeometryRecord>().get(fh_sd_name,info.topo_fh);
    es.get<IdealGeometryRecord>().get(bh_sd_name,info.topo_bh);
    triggerGeometry_->initialize(info);
}

/*****************************************************************/
void HGCalTriggerDigiFEReproducer::produce(edm::Event& e, const edm::EventSetup& es)
/*****************************************************************/
{
    std::unique_ptr<l1t::HGCFETriggerDigiCollection> fe_output( new l1t::HGCFETriggerDigiCollection );

    edm::Handle<l1t::HGCFETriggerDigiCollection> digis_h;

    e.getByToken(inputdigi_,digis_h);

    const l1t::HGCFETriggerDigiCollection& digis = *digis_h;

    fe_output->reserve(digis.size());
    std::stringstream output;
    for( const auto& digi_in : digis ) 
    {    
        fe_output->push_back(l1t::HGCFETriggerDigi());
        l1t::HGCFETriggerDigi& digi_out = fe_output->back();
        codec_->setDataPayload(digi_in);
        codec_->encode(digi_out);
        digi_out.setDetId( digi_in.getDetId<HGCalDetId>() );
        codec_->print(digi_out,output);
        edm::LogInfo("HGCalTriggerDigiFEReproducer")
            << output.str();
        codec_->unSetDataPayload();
        output.str(std::string());
        output.clear();
    }

    // get the orphan handle and fe digi collection
    auto fe_digis_handle = e.put(std::move(fe_output));
    auto fe_digis_coll = *fe_digis_handle;

    //now we run the emulation of the back-end processor
    backEndProcessor_->run(fe_digis_coll);
    backEndProcessor_->putInEvent(e);
    backEndProcessor_->reset();  
}
