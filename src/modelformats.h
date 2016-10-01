#ifndef MODELFORMATS_H
#define MODELFORMATS_H

#include <QVector>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QGL>
#include <QDebug>

// this is pretty fragile - if you aren't using Blender use a better parser
inline QVector<GLfloat> readObj(const QString &filename)
{
    QVector<GLfloat> result;

    QFile inputFile(filename);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QVector<GLfloat> verts, uvs;

        QTextStream in(&inputFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            QStringList data = line.split(" ");

            if (data[0] == "v")
            {
                verts.append(data[1].toFloat());
                verts.append(data[2].toFloat());
                verts.append(data[3].toFloat());
            }
            else if (data[0] == "vt")
            {
                uvs.append(data[1].toFloat());
                uvs.append(data[2].toFloat());
            }
            else if (data[0] == "f")
            {
                if (data.length() == 5) // quads
                {
                    // triangulate in the laziest way possible
                    int tris[] = { 0, 1, 2, 0, 2, 3 };
                    QStringList row;

                    for (int i=0; i<6; i++)
                    {
                        row = data[tris[i]+1].split("/");
                        int v = row[0].toInt()-1;
                        int vt = row[1].toInt()-1;
                        for (int j=0; j<3; j++)
                        {
                            result.append(verts.at(v*3+j));
                        }
                        for (int j=0; j<2; j++)
                        {
                            result.append(uvs.at(vt*2+j));
                        }
                    }
                }
                else
                {
                    QStringList row;
                    for (int i=1; i<4; i++)
                    {
                        row = data[i].split("/");
                        int v = row[0].toInt()-1;
                        int vt = row[1].toInt()-1;
                        for (int j=0; j<3; j++)
                        {
                            result.append(verts.at(v*3+j));
                        }
                        for (int j=0; j<2; j++)
                        {
                            result.append(uvs.at(vt*2+j));
                        }
                    }
                }
            }
        }
    }

    return result;
}

#endif // MODELFORMATS_H
