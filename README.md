# MetaHuman Performance Capture

This is a work in progress MetaHuman Creator pipeline that will assemble MetaHuman blueprints that can be used with the new Performance Capture Workflow available in Unreal 5.6.


# Instructions

- Download and extract into `ProjectName/Plugins`.
- Open your Project and in `Projecect Settings->MetaHuman Creator Build`, set the Cinematic legacy pipeline to `BP_PerfCapMetaHumanLegacyPipeline`.
- Open your MetaHuman asset in MetaHuman creator and in the Assembly section, choose `UE Cine` from the Assembly dropdown. Click `Assemble`.
- From the toolbar, open `Window->Virtual Production->Mocap Manager`. In the `Motion->Performers` section, create or use an existing performer asset.
- Go to `Motion->Characters` and open the `Create new character asset` section. For `Character Class` pick your assembled MetaHuman blueprint asset. For `Character Mesh`, use the body mesh that was generated during the MetaHuman assembly. Set your `Source Performer` asset and set or leave the other properties then click `Create Character Asset` then `Spawn Existing Character` up top.

# Notes

- If you're using a MetaHuman (Video) LiveLink source to drive the face in combination with another Live Link source to drive the body, you'll need to create a virtual LiveLink subject that combines your body motion capture subject with the face subject. 
- Make sure in the MetHuman (Video) source to disable head rotation and translation since this will be driven by your motion capture Live Link source.
