/* 3DMeshMetric is a visualization tool used to measures and display
 * surface-to-surface distance between two triangle meshes.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors : Juliette Pera, Francois Budin, Beatriz Paniagua
 * Web site : http://www.nitrc.org/projects/meshmetric3d/
 *
 * Reference : MeshValmet, Validation Metric for Meshes
 * http://www.nitrc.org/projects/meshvalmet/
 *
 * This program also uses Qt libraries
 * http://www.qt-project.org/
 */

#include "processing.h"

processing::processing()
{
}

//*************************************************************************************************
vtkSmartPointer <vtkPolyData> processing::processSmoothing( vtkSmartPointer <vtkPolyData> polyData , int nbIteration )
{
    vtkSmartPointer <vtkPolyData> SmoothedData = vtkSmartPointer<vtkPolyData>::New();
    vtkSmartPointer <vtkSmoothPolyDataFilter> smoother = vtkSmartPointer <vtkSmoothPolyDataFilter>::New();

    smoother -> SetInputData( polyData );
    smoother -> SetNumberOfIterations( nbIteration );
    smoother -> Update();

    SmoothedData =  smoother -> GetOutput();

    if( polyData -> GetNumberOfPoints() != SmoothedData -> GetNumberOfPoints() )
    {
        QMessageBox MsgBox;
        MsgBox.setText( " ERROR : PROCESSING SMOOTHING ");
        MsgBox.exec();
        return polyData;
    }
    return SmoothedData;
}


//*************************************************************************************************
vtkSmartPointer <vtkPolyData> processing::processDownSampling(vtkSmartPointer<vtkPolyData> polyData , double nbDecimate )
{
    vtkSmartPointer <vtkPolyData> DecimatedData = vtkSmartPointer <vtkPolyData>::New();
    vtkSmartPointer <vtkDecimatePro> Decimer = vtkSmartPointer <vtkDecimatePro>::New();

    if( polyData -> GetNumberOfPoints() == 0 )
    {
        QMessageBox MsgBox;
        MsgBox.setText( " ERROR : No data to decimate ");
        MsgBox.exec();
        return polyData;
    }

    Decimer -> SetInputData( polyData );
    Decimer -> SetTargetReduction( nbDecimate );
    Decimer -> PreserveTopologyOn();
    Decimer -> Update();

    DecimatedData = Decimer -> GetOutput();

    if( DecimatedData -> GetNumberOfPoints() == 0 )
    {
        QMessageBox MsgBox;
        MsgBox.setText( " ERROR : PROCESSING DOWN SAMPLING ");
        MsgBox.exec();
        return polyData;
    }
    return DecimatedData;
}


//*************************************************************************************************
int processing::processError( dataM &Data1 , dataM &Data2 )
{
    if( Data1.getTypeDistance() == 0 || Data1.getTypeDistance() == 1 )
    {
        vtkSmartPointer <vtkPolyData> ErrorData = vtkSmartPointer<vtkPolyData>::New();
        vtkSmartPointer <vtkColorTransferFunction> ErrorLut = vtkSmartPointer <vtkColorTransferFunction>::New();

        m_MyMeshValmet.SetData1( Data1.getPolyData() );
        m_MyMeshValmet.SetData2( Data2.getPolyData() );
        m_MyMeshValmet.SetSignedDistance( Data1.getTypeDistance() );
        m_MyMeshValmet.SetSamplingStep( Data1.getSamplingStep() );
        m_MyMeshValmet.SetMinSampleFrequency( Data1.getMinSamplingFrequency() );

        m_MyMeshValmet.CalculateError();

        ErrorData = m_MyMeshValmet.GetFinalData();
        ErrorLut = m_MyMeshValmet.GetLut();

        Data1.setPolyData( ErrorData );
        Data1.setLut( ErrorLut );

        Data1.setMin( m_MyMeshValmet.GetMin() );
        Data1.setMax( m_MyMeshValmet.GetMax() );
        Data1.setCenter( m_MyMeshValmet.GetCenter() );
        Data1.setDelta( m_MyMeshValmet.GetDelta() );

        Data1.changeMapperInput();

        return 0;
    }
    else if( Data1.getTypeDistance() == 2 )
    {
        if( Data1.getPolyData()->GetNumberOfPoints() == Data2.getPolyData()->GetNumberOfPoints() )
        {
            vtkSmartPointer <vtkPolyData> ErrorData = vtkSmartPointer<vtkPolyData>::New();
            vtkSmartPointer <vtkColorTransferFunction> ErrorLut = vtkSmartPointer <vtkColorTransferFunction>::New();
            vtkSmartPointer <vtkDoubleArray> ScalarsError = vtkSmartPointer <vtkDoubleArray>::New();
            vtkSmartPointer <vtkDoubleArray> ScalarsConst = vtkSmartPointer <vtkDoubleArray>::New();

            double Point1[3];
            double Point2[3];
            double Diff[3];
            double Result;

            ErrorData = Data1.getPolyData();

            for( vtkIdType Id = 0 ; Id < Data1.getPolyData()->GetNumberOfPoints() ; Id++ )
            {
                Data1.getPolyData()->GetPoints()->GetPoint( Id , Point1 );
                Data2.getPolyData()->GetPoints()->GetPoint( Id , Point2 );

                Diff[0] = Point1[0]-Point2[0];
                Diff[1] = Point1[1]-Point2[1];
                Diff[2] = Point1[2]-Point2[2];

                Result = sqrt( Diff[0]*Diff[0] + Diff[1]*Diff[1] + Diff[2]*Diff[2] );

                ScalarsError->InsertTuple1( Id , Result );
                ScalarsConst->InsertTuple1( Id , 1.0 );
            }

            ScalarsError->SetName( "Correspondant");
            ScalarsConst->SetName( "Original" );

            ErrorData->GetPointData()->AddArray( ScalarsError );
            ErrorData->GetPointData()->AddArray( ScalarsConst );

            Data1.setPolyData( ErrorData );

            double range[2];
            ScalarsError-> GetRange( range );

            Data1.setMin( range[0] );
            Data1.setMax( range[1] );

            // check for delta
            double Delta = CheckForDelta( range[1] , range[0] );
            Data1.setDelta( Delta );

            //check for center
            Data1.setCenter( range[0] );

            ErrorLut -> SetColorSpaceToRGB();

            ErrorLut -> AddRGBSegment( range[0] , 0 , 1 , 0 , range[0] + Data1.getDelta() , 1 , 1 , 0 );
            ErrorLut -> AddRGBSegment( range[0] + Data1.getDelta() , 1 , 1 , 0 , range[1] , 1 , 0 , 0 );

            Data1.setLut( ErrorLut );

            Data1.changeMapperInput();

            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}


//*************************************************************************************************
int processing::CheckPreviousError( dataM &Data1 )
{
    vtkSmartPointer <vtkPolyData> PolyData = vtkSmartPointer <vtkPolyData>::New();
    PolyData = Data1.getPolyData();

    int NumberOfArrays = PolyData -> GetPointData() -> GetNumberOfArrays();
    int Check = 0;
    int Indice = 0;

    if( NumberOfArrays == 0 ) // no array
    {
        return -1;
    }
    else // some arrays
    {
        const char* Names ;
        const char* NameDistance = 0;
        for( int i = 0; i < NumberOfArrays ; i++ )
        {
            Names = PolyData -> GetPointData() -> GetArrayName( i );

            if( strcmp( Names , "Signed" ) == 0 || strcmp( Names , "Absolute" ) == 0 || strcmp( Names , "Correspondant" ) == 0 )
            {
                Check = Check + 1;
                Indice = i;
                NameDistance = Names ;
            }
            if( strcmp( Names , "Original" ) == 0 )
            {
                Check = Check + 2;
            }
        }

        if( Check == 3 ) // both error and original are present
        {
            vtkDataArray* Array = vtkDataArray::SafeDownCast( PolyData -> GetPointData() -> GetArray( Indice ) );

            double range[2];
            Array -> GetRange( range );

            Data1.setMax( range[ 1 ] );

            double Delta = CheckForDelta( range[1] , range[0] );
            Data1.setDelta( Delta );

            if( strcmp( NameDistance , "Signed" ) == 0 )
            {
                Data1.setMin( range[ 0 ] );
                Data1.setCenter( (range[1] + range[0])/2.0 );

                Data1.setTypeDistance( 1 );
                return 31;
            }
            else if( strcmp( NameDistance , "Absolute" ) == 0 )
            {
                if( range[ 0 ] >= 0 )
                {
                    Data1.setMin( range[ 0 ] );
                    Data1.setCenter( range[ 0 ] );
                }
                else
                {
                    Data1.setMin( 0 );
                    Data1.setCenter( 0 );
                }

                Data1.setTypeDistance( 0 );
                return 32;
            }
            else if( strcmp( NameDistance , "Correspondant" ) == 0 )
            {
                if( range[ 0 ] >= 0 )
                {
                    Data1.setMin( range[ 0 ] );
                    Data1.setCenter( range[ 0 ] );
                }
                else
                {
                    Data1.setMin( 0 );
                    Data1.setCenter( 0 );
                }

                Data1.setTypeDistance( 2 );
                return 33;
            }
            return 3; // -> pb
        }
        else if( Check == 0 || Check == 1 || Check == 2 )
        {
            // there is some arrays but no error AND original
            return 0;
        }
        else
        {         
            return 4; // pb
        }
    }

}


//*************************************************************************************************
int processing::SaveFile( std::string Name, dataM &Data1 , int format )
{

    if( format == 1 )
    {
        vtkSmartPointer <vtkPolyDataWriter> WriterVTK = vtkSmartPointer <vtkPolyDataWriter>::New();
        WriterVTK -> SetInputData( Data1.getPolyData() );
        WriterVTK -> SetFileName( Name.c_str() );
        WriterVTK -> Update();
        return 0;
    }
    else if( format == 2 )
    {
        vtkSmartPointer <vtkOBJWriter> WriterOBJ = vtkSmartPointer <vtkOBJWriter>::New();
        WriterOBJ -> SetInputData( Data1.getPolyData() );
        WriterOBJ -> SetFileName( Name.c_str() );
        WriterOBJ -> Update();

        return 0;
    }
    else if( format == 3 )
    {
        vtkSmartPointer <vtkSTLWriter> WriterSTL = vtkSmartPointer <vtkSTLWriter>::New();
        WriterSTL -> SetInputData( Data1.getPolyData() );
        WriterSTL -> SetFileName( Name.c_str() );
        WriterSTL -> Update();
        return 0;
    }
    else if( format == 4 )
    {
        vtkSmartPointer <vtkXMLPolyDataWriter> WriterVTP = vtkSmartPointer <vtkXMLPolyDataWriter>::New();
        WriterVTP -> SetInputData( Data1.getPolyData() );
        WriterVTP -> SetFileName( Name.c_str() );
        WriterVTP -> SetDataModeToBinary();
        WriterVTP -> Update();
        return 0;
    }
    else
    {
        return -1;
    }
}


//*************************************************************************************************
void processing::updateColor( dataM &Data1 )
{
    vtkSmartPointer <vtkColorTransferFunction> ErrorLut = vtkSmartPointer <vtkColorTransferFunction>::New();

    ErrorLut -> SetColorSpaceToRGB();

    if( Data1.getTypeDistance() == 1 )
    {   
         ErrorLut -> AddRGBSegment( Data1.getMin() , 0 , 0 , 1 , Data1.getCenter()-Data1.getDelta() , 0 , 1 , 1 );
         ErrorLut -> AddRGBSegment( Data1.getCenter()-Data1.getDelta() , 0 , 1 , 1 , Data1.getCenter()  , 0 , 1 , 0 );
         ErrorLut -> AddRGBSegment( Data1.getCenter() , 0 , 1 , 0 , Data1.getCenter()+Data1.getDelta() , 1 , 1 , 0 );
         ErrorLut -> AddRGBSegment( Data1.getCenter()+Data1.getDelta() , 1 , 1 , 0 , Data1.getMax() , 1 , 0 , 0 );
    }
    else if( Data1.getTypeDistance() == 2 || Data1.getTypeDistance() == 0 )
    {
         ErrorLut -> AddRGBSegment( Data1.getMin() , 0 , 1 , 0 , Data1.getMin()+Data1.getDelta() , 1 , 1 , 0 );
         ErrorLut -> AddRGBSegment( Data1.getMin()+Data1.getDelta() , 1 , 1 , 0 , Data1.getMax() , 1 , 0 , 0 );
    }

    Data1.setLut( ErrorLut );

    Data1.changeActivScalar();
}


//*************************************************************************************************
int processing::testPolyData( vtkSmartPointer <vtkPolyData> inData , vtkSmartPointer <vtkPolyData> outData )
{
    // comparer pointeurs
    std::cout << "          -number of points  " << std::endl;
    if( outData -> GetNumberOfPoints() != inData -> GetNumberOfPoints() )
    {
        std::cout << " ERROR : number of points " << std::endl;
        return 1;
    }

    std::cout << "          -number of cells  " << std::endl;
    if( outData -> GetNumberOfCells() != inData -> GetNumberOfCells() )
    {
        std::cout << " ERROR : number of cells " << std::endl;
        return 1;
    }
    vtkIdType b = outData -> GetNumberOfCells();


    std::cout << "          -get points  " << std::endl;
    vtkIdType Id = 0;
    double inV[3];
    double outV[3];

    for( Id = 0 ; Id < inData -> GetNumberOfPoints() ; Id++ )
    {
        inData -> GetPoints() -> GetPoint( Id , inV );
        outData -> GetPoints() -> GetPoint( Id , outV );
        if( inV[0] != outV[0] || inV[1] != outV[1] || inV[2] != outV[2] )
        {
            std::cout << " ERROR :  points " << std::endl;
            return 1;
        }

    }

    std::cout << "          -get cells  " << std::endl;
    vtkSmartPointer <vtkIdList> inF = vtkSmartPointer <vtkIdList>::New();
    vtkSmartPointer <vtkIdList> outF = vtkSmartPointer <vtkIdList>::New();
    vtkSmartPointer <vtkCellArray> inPolys = vtkSmartPointer <vtkCellArray> ::New();
    vtkSmartPointer <vtkCellArray> outPolys = vtkSmartPointer <vtkCellArray> ::New();
    inPolys = inData -> GetPolys();
    outPolys = outData -> GetPolys();

    for( Id = 0 ; Id < b ; Id++ )
    {
        inPolys -> GetCell( Id , inF );
        outPolys -> GetCell( Id , outF );

        vtkIdType c = inF->GetNumberOfIds();
        vtkIdType d = outF->GetNumberOfIds();
        if( c!= d && c != 3 )
        {
            return 1 ;
        }
        for( vtkIdType i = 0 ; i < c ; i++ )
        {
            if( inF->GetId(i) != outF->GetId(i) )
            {
                std::cout << " ERROR :  cells " << std::endl;
                return 1;
            }
        }
    }
    return 0;
}

double processing::CheckForDelta( double Rmax , double Rmin )
{
    double Delta = ( Rmax - Rmin )/2.0 ;
    if( Delta >= 0.5 )
    {
        Delta = 0.5;
    }
    else
    {
        Delta = 0.02;
    }
    return Delta;
}


