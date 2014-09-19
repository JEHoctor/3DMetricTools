// My library
#include "ModelToModelDistanceCLP.h"
#ifdef USE_VTK_FILTER
#include <vtkDistancePolyDataFilter.h>
#else
#include "MeshValmet.h"
#endif
#include <vtkVersion.h>
#include <vtkPolyDataWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkCommand.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkCleanPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkPolyDataNormals.h>
#include <vtkFloatArray.h>

//class ErrorObserver copied from http://www.vtk.org/Wiki/VTK/Examples/Cxx/Utilities/ObserveError
class ErrorObserver : public vtkCommand
{
public:
    ErrorObserver():
        Error(false),
        Warning(false),
        ErrorMessage(""),
        WarningMessage("") {}
    static ErrorObserver *New()
    {
        return new ErrorObserver;
    }
    bool GetError() const
    {
        return this->Error;
    }
    bool GetWarning() const
    {
        return this->Warning;
    }
    void Clear()
    {
        this->Error = false;
        this->Warning = false;
        this->ErrorMessage = "";
        this->WarningMessage = "";
    }
    virtual void Execute(vtkObject *vtkNotUsed(caller),
                         unsigned long event,
                         void *calldata)
    {
        switch(event)
        {
        case vtkCommand::ErrorEvent:
            ErrorMessage = static_cast<char *>(calldata);
            this->Error = true;
            break;
        case vtkCommand::WarningEvent:
            WarningMessage = static_cast<char *>(calldata);
            this->Warning = true;
            break;
        }
    }
    std::string GetErrorMessage()
    {
        return ErrorMessage;
    }
    std::string GetWarningMessage()
    {
        return WarningMessage;
    }
private:
    bool        Error;
    bool        Warning;
    std::string ErrorMessage;
    std::string WarningMessage;
};


int TriangulateAndClean(vtkSmartPointer<vtkPolyData> &polyData )
{
    vtkSmartPointer <vtkCleanPolyData> Cleaner = vtkSmartPointer <vtkCleanPolyData>::New() ;
#if VTK_MAJOR_VERSION > 5
    Cleaner->SetInputData( polyData ) ;
#else
    Cleaner->SetInput( polyData ) ;
#endif
    Cleaner->Update() ;
    vtkSmartPointer <vtkTriangleFilter> Triangler = vtkSmartPointer <vtkTriangleFilter>::New() ;
#if VTK_MAJOR_VERSION > 5
    Triangler->SetInputData( Cleaner->GetOutput() ) ;
#else
    Triangler->SetInput( Cleaner->GetOutput() ) ;
#endif
    Triangler->Update() ;
    polyData = Triangler->GetOutput() ;
    return 0 ;
}


void PointsToVec( double p1[] , double p2[] , double vec[] )
{
    for( int i = 0 ; i < 3 ; i++ )
    {
        vec[ i ] = p2[ i ] - p1[ i ] ;
    }
}

int CorrespondingPointsDistance( vtkSmartPointer< vtkPolyData > &inPolyData1 ,
                                 vtkSmartPointer< vtkPolyData > &inPolyData2
                                 )
{
    vtkIdType nbPoints1 = inPolyData1->GetNumberOfPoints() ;
    vtkIdType nbPoints2 = inPolyData2->GetNumberOfPoints() ;
    if( nbPoints1 != nbPoints2 )
    {
        std::cerr << "Both input files must have the same number of points to use \'corresponding_point_to_point\'" << std::endl ;
        return 1 ;
    }
    //Compute normals on inPolyData1
    vtkSmartPointer< vtkPolyDataNormals > normalGenerator = vtkSmartPointer<vtkPolyDataNormals>::New() ;
    #if ( VTK_MAJOR_VERSION < 6 )
    normalGenerator->SetInput( inPolyData1 );
    #else
    normalGenerator->SetInputData( inPolyData1 ) ;
    #endif
    normalGenerator->SplittingOff() ;
    normalGenerator->ComputePointNormalsOn() ;
    normalGenerator->ComputeCellNormalsOff() ;
    normalGenerator->Update() ;
    inPolyData1 = normalGenerator->GetOutput() ;
    //vtkSmartPointer< vtkDataSet > normals = inPolyData1->GetPointData()->GetArray( "Normals" ) ;
    vtkSmartPointer< vtkFloatArray > normalsArray = vtkFloatArray::SafeDownCast( inPolyData1->GetPointData()->GetNormals() ) ;
    /////////////////////////////////
    vtkSmartPointer <vtkDoubleArray> distanceArray = vtkSmartPointer <vtkDoubleArray>::New();
    distanceArray->SetName( "AbsolutePointToPointDistance" ) ;
    vtkSmartPointer <vtkDoubleArray> distanceVecArray = vtkSmartPointer <vtkDoubleArray>::New();
    distanceVecArray->SetNumberOfComponents( 3 ) ;
    distanceVecArray->SetName( "PointToPointVector" ) ;
    vtkSmartPointer <vtkDoubleArray> signedDistanceArray = vtkSmartPointer <vtkDoubleArray>::New();
    signedDistanceArray->SetName( "SignedPointToPointDistance" ) ;
    vtkSmartPointer <vtkDoubleArray> signedMagNormDirArray = vtkSmartPointer <vtkDoubleArray>::New();
    signedMagNormDirArray->SetName( "SignedMagNormDirDistance" ) ;
    vtkSmartPointer <vtkDoubleArray> magNormDirArray = vtkSmartPointer <vtkDoubleArray>::New();
    magNormDirArray->SetName( "AbsoluteMagNormDirDistance" ) ;
    vtkSmartPointer <vtkDoubleArray> magNormDirVecArray = vtkSmartPointer <vtkDoubleArray>::New();
    magNormDirVecArray->SetNumberOfComponents( 3 ) ;
    magNormDirVecArray->SetName( "MagNormVector" ) ;
    double absoluteDistance ;
    double* p1 ;
    double* p2 ;
    double vec[ 3 ] ;
    double* normal ;
    double dotProduct ;
    for( vtkIdType i = 0 ; i < nbPoints1 ; i++ )
    {
        p1 = inPolyData1->GetPoint( i ) ;
        p2 = inPolyData2->GetPoint( i ) ;
        PointsToVec( p1 , p2 , vec ) ;
        normal = normalsArray->GetTuple( i ) ;
        dotProduct = vtkMath::Dot( normal , vec ) ;
        //Signed and absolute corresponding point distance
        absoluteDistance = vtkMath::Norm( vec ) ;
        signedDistanceArray->InsertTuple1( i , ( dotProduct >= 0 ? absoluteDistance : -absoluteDistance ) ) ;
        distanceArray->InsertTuple1( i , absoluteDistance ) ;
        distanceVecArray->InsertTuple3( i , vec[ 0 ] , vec[ 1 ] , vec[ 2 ] ) ;
        //MagDir: projection of the distance vector on the normal
        signedMagNormDirArray->InsertTuple1( i , dotProduct ) ;
        magNormDirArray->InsertTuple1( i , ( dotProduct >= 0 ? dotProduct : -dotProduct ) ) ;
        for( int j = 0 ; j < 3 ; j++ )
        {
            normal[ j ] *= dotProduct ;
        }
        magNormDirVecArray->InsertTuple3( i , normal[ 0 ] , normal[ 1 ] , normal[ 2 ] ) ;
    }
    inPolyData1->GetPointData()->AddArray( distanceArray ) ;
    inPolyData1->GetPointData()->AddArray( magNormDirVecArray ) ;
    inPolyData1->GetPointData()->AddArray( distanceVecArray ) ;
    inPolyData1->GetPointData()->AddArray( signedDistanceArray ) ;
    inPolyData1->GetPointData()->AddArray( signedMagNormDirArray ) ;
    inPolyData1->GetPointData()->AddArray( magNormDirArray ) ;
    return 0 ;
}

int ClosestPointDistance( vtkSmartPointer< vtkPolyData > &inPolyData1 ,
                          vtkSmartPointer< vtkPolyData > &inPolyData2 ,
                          bool signedDistance ,
                          vtkSmartPointer< vtkPolyData > &outPolyData
                          )
{
#ifdef USE_VTK_FILTER
    vtkSmartPointer<vtkDistancePolyDataFilter> distanceFilter =
            vtkSmartPointer<vtkDistancePolyDataFilter>::New();
#if VTK_MAJOR_VERSION > 5
    distanceFilter->SetInputData( 0, inPolyData1 ) ;
    distanceFilter->SetInputData( 1, inPolyData2 ) ;
#else
    distanceFilter->SetInput( 0, inPolyData1 ) ;
    distanceFilter->SetInput( 1, inPolyData2 ) ;
#endif
    distanceFilter->SetSignedDistance( signedDistance ) ;
    distanceFilter->Update();
    //We are only interested in the point distance, not in the cell distance, so we remove the cell distance
    distanceFilter->GetOutput()->GetCellData()->RemoveArray("Distance") ;
    //We rename the output array name to match the expected names in 3DMeshMetric
    std::string distanceName ;
    if( signedDistance )
    {
        distanceName = "Signed" ;
    }
    else
    {
        distanceName = "Absolute" ;
    }
    distanceFilter->GetOutput()->GetPointData()->GetArray("Distance")->SetName(distanceName.c_str()) ;
    //We add a constant field that we call "original" that allows to show easily the model with no color map (constant color)
    vtkSmartPointer<vtkPolyData> distancePolyData = distanceFilter->GetOutput() ;
    vtkSmartPointer <vtkDoubleArray> ScalarsConst = vtkSmartPointer <vtkDoubleArray>::New();
    for( vtkIdType Id = 0 ; Id < distancePolyData->GetNumberOfPoints() ; Id++ )
    {
        ScalarsConst->InsertTuple1( Id , 1.0 );
    }
    ScalarsConst->SetName( "Original" );
    distancePolyData->GetPointData()->AddArray( ScalarsConst );
    vtkSmartPointer <vtkCleanPolyData> Cleaner = vtkSmartPointer <vtkCleanPolyData>::New() ;
#if VTK_MAJOR_VERSION > 5
    Cleaner->SetInputData( distancePolyData ) ;
#else
    Cleaner->SetInput( distancePolyData ) ;
#endif
    Cleaner->Update() ;
#else
    meshValmet errorComputeFilter ;
    errorComputeFilter.SetData1( inPolyData1 );
    errorComputeFilter.SetData2( inPolyData2 );
    errorComputeFilter.SetSignedDistance( signedDistance ) ;
    errorComputeFilter.SetSamplingStep( samplingStep ) ;
    errorComputeFilter.SetMinSampleFrequency( minSampleFrequency ) ;
    errorComputeFilter.CalculateError() ;
    vtkSmartPointer <vtkCleanPolyData> Cleaner = vtkSmartPointer <vtkCleanPolyData>::New() ;
#if VTK_MAJOR_VERSION > 5
    Cleaner->SetInputData( errorComputeFilter.GetFinalData() ) ;
#else
    Cleaner->SetInput( errorComputeFilter.GetFinalData() ) ;
#endif
    Cleaner->Update() ;
#endif
    outPolyData = Cleaner->GetOutput() ;
    return 0 ;
}

int ReadVTK( std::string input , vtkSmartPointer<vtkPolyData> &polyData )
{
    vtkSmartPointer<ErrorObserver>  errorObserver =
            vtkSmartPointer<ErrorObserver>::New();
    if( input.rfind( ".vtk" ) != std::string::npos )
    {
        vtkSmartPointer< vtkPolyDataReader > polyReader = vtkPolyDataReader::New() ;
        polyReader->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
        polyReader->SetFileName( input.c_str() ) ;
        polyData = polyReader->GetOutput() ;
        polyReader->Update() ;
    }
    else if( input.rfind( ".vtp" ) != std::string::npos )
    {
        vtkSmartPointer< vtkXMLPolyDataReader > xmlReader = vtkXMLPolyDataReader::New() ;
        xmlReader->SetFileName( input.c_str() ) ;
        xmlReader->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
        polyData = xmlReader->GetOutput() ;
        xmlReader->Update() ;
    }
    else
    {
        std::cerr << "Input file format not handled: " << input << " cannot be read" << std::endl ;
        return 1 ;
    }
    if (errorObserver->GetError())
    {
        std::cout << "Caught error opening " << input << std::endl ;
        return 1 ;
    }
    return 0 ;
}

int WriteVTK( std::string output , vtkSmartPointer<vtkPolyData> &polyData )
{
    vtkSmartPointer<ErrorObserver>  errorObserver =
            vtkSmartPointer<ErrorObserver>::New();
    if (output.rfind(".vtk") != std::string::npos )
    {
        vtkSmartPointer<vtkPolyDataWriter> writer = vtkPolyDataWriter::New() ;
        writer->SetFileName( output.c_str() ) ;
        writer->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
#if VTK_MAJOR_VERSION > 5
        writer->SetInputData( polyData ) ;
#else
        writer->SetInput( polyData ) ;
#endif
        writer->Update();
    }
    else if( output.rfind( ".vtp" ) != std::string::npos )
    {
        vtkSmartPointer< vtkXMLPolyDataWriter > writer = vtkXMLPolyDataWriter::New() ;
        writer->SetFileName( output.c_str() ) ;
        writer->AddObserver( vtkCommand::ErrorEvent , errorObserver ) ;
#if VTK_MAJOR_VERSION > 5
        writer->SetInputData( polyData ) ;
#else
        writer->SetInput( polyData ) ;
#endif
        writer->Update() ;
    }
    else
    {
        std::cerr << "Output file format not handled: " << output << " cannot be written" << std::endl ;
        return 1 ;
    }
    if (errorObserver->GetError())
    {
        std::cout << "Caught error saving " << output << std::endl ;
        return 1 ;
    }
    return 0 ;
}

int main( int argc , char* argv[] )
{
    PARSE_ARGS ;
    if( vtkFile1.empty() || vtkFile2.empty() )
    {
        std::cout << "Specify 2 vtk input files" << std::endl ;
        return 1 ;
    }
    if( vtkOutput.empty() )
    {
        std::cout << "Specify an output file name" << std::endl ;
        return 1 ;
    }
    vtkSmartPointer<vtkPolyData> inPolyData1 = vtkSmartPointer<vtkPolyData>::New() ;
    if( ReadVTK( vtkFile1 , inPolyData1 ) )
    {
        return 1 ;
    }
    vtkSmartPointer<vtkPolyData> inPolyData2 = vtkSmartPointer<vtkPolyData>::New() ;
    if( ReadVTK( vtkFile2 , inPolyData2 ) )
    {
        return 1 ;
    }
    if( TriangulateAndClean( inPolyData1 ) )
    {
        return EXIT_FAILURE ;
    }
    if( TriangulateAndClean( inPolyData2 ) )
    {
        return EXIT_FAILURE ;
    }
    vtkSmartPointer< vtkPolyData > outPolyData ;
    if( distanceType == "corresponding_point_to_point" )
    {
        if( CorrespondingPointsDistance( inPolyData1 , inPolyData2 ) )
        {
            return EXIT_FAILURE ;
        }
        outPolyData = inPolyData1 ;
    }
    else
    {
        bool signedDistance = false ;
        if( distanceType == "signed_closest_point" )
        {
            signedDistance = true ;
        }
        if( ClosestPointDistance( inPolyData1 , inPolyData2 , signedDistance , outPolyData ) )
        {
            return EXIT_FAILURE ;
        }
    }

    return WriteVTK( vtkOutput , outPolyData ) ;
}
