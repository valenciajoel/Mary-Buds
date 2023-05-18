package com.example.autentificarse;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

public class MainActivity extends AppCompatActivity {


    TextView mail;
    TextView dato;
    private DatabaseReference mDatabase;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main2);

        mail = findViewById(R.id.mail);
        dato = findViewById(R.id.dato);

        mDatabase = FirebaseDatabase.getInstance().getReference();

        mDatabase.child("/test/int").addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                String valor = snapshot.getValue().toString();
                dato.setText(valor);
            }

            @Override
            public void onCancelled(@NonNull DatabaseError error) {

            }
        });


        mail.setText(getIntent().getStringExtra("mail"));
        //dato.setText();




    }
}