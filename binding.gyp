{
  "targets": [
    {
      "target_name": "nodedbi",
      "sources": [ "src/connection.cpp", "src/query.cpp" ],
      'cflags': [
        '<!@(pkg-config --cflags dbi)'
      ],
      'ldflags': [
        '<!@(pkg-config --libs-only-L --libs-only-other dbi)'
      ],
      'libraries': [
        '<!@(pkg-config --libs-only-l dbi)'
      ]
    }
  ]
}
