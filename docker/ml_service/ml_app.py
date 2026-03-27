import streamlit as st
import matlab.engine

if 'meng' not in st.session_state:
    st.session_state['meng'] = matlab.engine.start_matlab()
eng = st.session_state['meng']

# rssi1 = st.number_input("RSSI 1", value=-50)
# rssi2 = st.number_input("RSSI 2", value=-50)
# rssi3 = st.number_input("RSSI 3", value=-50)
if st.button("Get location"):
    area_id = eng.ml_lc(rssi1, rssi2, rssi3)
    st.write(int(area_id))
else:
    st.write('Fill RSSI, then press button')